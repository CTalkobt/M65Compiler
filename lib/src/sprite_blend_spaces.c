/* sprite_blend_spaces.c — 2D Animation Blend Spaces Implementation
 *
 * Provides parameter-based animation blending for procedural animation control.
 */

#include <sprite_blend_spaces.h>
#include <sprite_animation_trees.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_BLEND_SPACES 32
#define MAX_BLEND_POINTS 64
#define MAX_NAME_LEN 64

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ========================================================================== */

typedef struct {
    float x, y, z;
    char state_name[MAX_NAME_LEN];
} blend_point_t;

typedef struct {
    sprite_anim_tree_t tree;
    int dimensions;
    blend_point_t points[MAX_BLEND_POINTS];
    int point_count;
    float param_x, param_y, param_z;
    sprite_blend_mode_t mode;
    float weights[MAX_BLEND_POINTS];
    int dirty;
} sprite_blend_space_data_t;

static sprite_blend_space_data_t g_blend_spaces[MAX_BLEND_SPACES];
static int g_blend_space_count = 0;

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

static int blend_space_index(sprite_blend_space_t space)
{
    if (!space) return -1;
    int idx = (intptr_t)space;
    return (idx >= 0 && idx < g_blend_space_count) ? idx : -1;
}

static sprite_blend_space_data_t *blend_space_get(sprite_blend_space_t space)
{
    int idx = blend_space_index(space);
    return (idx < 0) ? NULL : &g_blend_spaces[idx];
}

static float distance_euclidean_2d(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

static float distance_euclidean_3d(float x1, float y1, float z1,
                                   float x2, float y2, float z2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static void recalculate_blend_weights(sprite_blend_space_data_t *space)
{
    if (space->point_count == 0) return;

    for (int i = 0; i < space->point_count; i++) {
        space->weights[i] = 0.0f;
    }

    if (space->point_count == 1) {
        space->weights[0] = 1.0f;
        return;
    }

    float distances[MAX_BLEND_POINTS];
    float min_dist = 1e9f;

    for (int i = 0; i < space->point_count; i++) {
        blend_point_t *p = &space->points[i];
        float dist;

        if (space->dimensions == 1) {
            dist = fabsf(space->param_x - p->x);
        } else if (space->dimensions == 2) {
            dist = distance_euclidean_2d(space->param_x, space->param_y,
                                        p->x, p->y);
        } else {
            dist = distance_euclidean_3d(space->param_x, space->param_y, space->param_z,
                                        p->x, p->y, p->z);
        }

        distances[i] = dist;
        if (dist < min_dist) min_dist = dist;
    }

    if (min_dist < 0.01f) {
        for (int i = 0; i < space->point_count; i++) {
            if (distances[i] < 0.01f) {
                space->weights[i] = 1.0f;
            }
        }
        return;
    }

    float total_weight = 0.0f;
    for (int i = 0; i < space->point_count; i++) {
        float weight = 1.0f / (distances[i] + 0.001f);
        space->weights[i] = weight;
        total_weight += weight;
    }

    if (total_weight > 0.001f) {
        for (int i = 0; i < space->point_count; i++) {
            space->weights[i] /= total_weight;
        }
    }

    space->dirty = 0;
}

/* ============================================================================
 * BLEND SPACE CREATION & CONTROL
 * ========================================================================== */

sprite_blend_space_t sprite_blend_space_create(sprite_anim_tree_t tree,
                                                int dimensions)
{
    if (!tree || dimensions < 1 || dimensions > 3) return INVALID_BLEND_SPACE;
    if (g_blend_space_count >= MAX_BLEND_SPACES) return INVALID_BLEND_SPACE;

    int idx = g_blend_space_count;
    sprite_blend_space_data_t *space = &g_blend_spaces[idx];

    space->tree = tree;
    space->dimensions = dimensions;
    space->point_count = 0;
    space->param_x = 0.0f;
    space->param_y = 0.0f;
    space->param_z = 0.0f;
    space->mode = BLEND_MODE_LINEAR;
    space->dirty = 1;

    g_blend_space_count++;
    return (sprite_blend_space_t)(intptr_t)idx;
}

void sprite_blend_space_destroy(sprite_blend_space_t space)
{
    int idx = blend_space_index(space);
    if (idx < 0) return;

    sprite_blend_space_data_t *s = &g_blend_spaces[idx];
    s->point_count = 0;
    s->tree = NULL;
}

int sprite_blend_space_get_info(sprite_blend_space_t space,
                                 sprite_blend_space_info_t *info)
{
    if (!info) return 0;
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    info->space = space;
    info->dimension_count = s->dimensions;
    info->point_count = s->point_count;
    info->param_x = s->param_x;
    info->param_y = s->param_y;
    info->param_z = s->param_z;
    info->mode = s->mode;
    info->dirty = s->dirty;
    return 1;
}

void sprite_blend_space_set_mode(sprite_blend_space_t space,
                                  sprite_blend_mode_t mode)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (s) s->mode = mode;
}

sprite_blend_mode_t sprite_blend_space_get_mode(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    return (s) ? s->mode : BLEND_MODE_LINEAR;
}

/* ============================================================================
 * BLEND POINT MANAGEMENT
 * ========================================================================== */

sprite_blend_point_t sprite_blend_space_add_point(sprite_blend_space_t space,
                                                   float x, float y, float z,
                                                   const char *state_name)
{
    if (!state_name) return INVALID_BLEND_POINT;
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s || s->point_count >= MAX_BLEND_POINTS) return INVALID_BLEND_POINT;

    int idx = s->point_count;
    blend_point_t *p = &s->points[idx];

    p->x = x;
    p->y = y;
    p->z = z;
    strncpy(p->state_name, state_name, MAX_NAME_LEN - 1);

    s->point_count++;
    s->dirty = 1;

    return (sprite_blend_point_t)(intptr_t)idx;
}

int sprite_blend_space_remove_point(sprite_blend_space_t space,
                                     sprite_blend_point_t point)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    int idx = (intptr_t)point;
    if (idx < 0 || idx >= s->point_count) return 0;

    for (int i = idx; i < s->point_count - 1; i++) {
        s->points[i] = s->points[i + 1];
    }
    s->point_count--;
    s->dirty = 1;
    return 1;
}

int sprite_blend_space_get_point_count(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    return (s) ? s->point_count : 0;
}

int sprite_blend_space_get_point(sprite_blend_space_t space, int index,
                                  sprite_blend_point_info_t *info)
{
    if (!info) return 0;
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s || index < 0 || index >= s->point_count) return 0;

    blend_point_t *p = &s->points[index];
    info->x = p->x;
    info->y = p->y;
    strncpy(info->state_name, p->state_name, MAX_NAME_LEN - 1);
    return 1;
}

sprite_blend_point_t sprite_blend_space_find_point(sprite_blend_space_t space,
                                                    const char *state_name)
{
    if (!state_name) return INVALID_BLEND_POINT;
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return INVALID_BLEND_POINT;

    for (int i = 0; i < s->point_count; i++) {
        if (strcmp(s->points[i].state_name, state_name) == 0) {
            return (sprite_blend_point_t)(intptr_t)i;
        }
    }
    return INVALID_BLEND_POINT;
}

int sprite_blend_space_move_point(sprite_blend_space_t space,
                                   sprite_blend_point_t point,
                                   float x, float y, float z)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    int idx = (intptr_t)point;
    if (idx < 0 || idx >= s->point_count) return 0;

    s->points[idx].x = x;
    s->points[idx].y = y;
    s->points[idx].z = z;
    s->dirty = 1;
    return 1;
}

/* ============================================================================
 * PARAMETER CONTROL
 * ========================================================================== */

int sprite_blend_space_set_parameters(sprite_blend_space_t space,
                                       float x, float y, float z)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    s->param_x = x;
    s->param_y = y;
    s->param_z = z;
    s->dirty = 1;
    return 1;
}

int sprite_blend_space_get_parameters(sprite_blend_space_t space,
                                       float *x, float *y, float *z)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    if (x) *x = s->param_x;
    if (y) *y = s->param_y;
    if (z) *z = s->param_z;
    return 1;
}

void sprite_blend_space_set_parameter_x(sprite_blend_space_t space, float value)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (s) {
        s->param_x = value;
        s->dirty = 1;
    }
}

void sprite_blend_space_set_parameter_y(sprite_blend_space_t space, float value)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (s) {
        s->param_y = value;
        s->dirty = 1;
    }
}

void sprite_blend_space_set_parameter_z(sprite_blend_space_t space, float value)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (s) {
        s->param_z = value;
        s->dirty = 1;
    }
}

float sprite_blend_space_get_parameter_x(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    return (s) ? s->param_x : 0.0f;
}

float sprite_blend_space_get_parameter_y(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    return (s) ? s->param_y : 0.0f;
}

float sprite_blend_space_get_parameter_z(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    return (s) ? s->param_z : 0.0f;
}

/* ============================================================================
 * BLENDING & INTERPOLATION
 * ========================================================================== */

int sprite_blend_space_update(sprite_blend_space_t space, int delta_ms)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s || !s->tree) return 0;

    if (s->dirty) {
        recalculate_blend_weights(s);
    }

    int updated = sprite_anim_tree_update(s->tree, delta_ms);
    return updated;
}

char *sprite_blend_space_get_active_state(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s || s->point_count == 0) return NULL;

    float max_weight = 0.0f;
    int max_idx = 0;

    for (int i = 0; i < s->point_count; i++) {
        if (s->weights[i] > max_weight) {
            max_weight = s->weights[i];
            max_idx = i;
        }
    }

    char *result = malloc(MAX_NAME_LEN);
    if (result) {
        strncpy(result, s->points[max_idx].state_name, MAX_NAME_LEN - 1);
    }
    return result;
}

int sprite_blend_space_get_blend_weights(sprite_blend_space_t space,
                                          float *weights, int max_weights)
{
    if (!weights || max_weights <= 0) return 0;
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0;

    if (s->dirty) {
        recalculate_blend_weights(s);
    }

    int count = (s->point_count < max_weights) ? s->point_count : max_weights;
    for (int i = 0; i < count; i++) {
        weights[i] = s->weights[i];
    }
    return count;
}

float sprite_blend_space_distance_to_point(sprite_blend_space_t space,
                                            sprite_blend_point_t point)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) return 0.0f;

    int idx = (intptr_t)point;
    if (idx < 0 || idx >= s->point_count) return 0.0f;

    blend_point_t *p = &s->points[idx];

    if (s->dimensions == 1) {
        return fabsf(s->param_x - p->x);
    } else if (s->dimensions == 2) {
        return distance_euclidean_2d(s->param_x, s->param_y, p->x, p->y);
    } else {
        return distance_euclidean_3d(s->param_x, s->param_y, s->param_z,
                                    p->x, p->y, p->z);
    }
}

/* ============================================================================
 * COMMON BLEND SPACE PATTERNS
 * ========================================================================== */

sprite_blend_space_t sprite_blend_space_create_1d_speed(sprite_anim_tree_t tree,
                                                         float idle_threshold,
                                                         float run_threshold,
                                                         const char *idle_state,
                                                         const char *walk_state,
                                                         const char *run_state)
{
    sprite_blend_space_t space = sprite_blend_space_create(tree, 1);
    if (space == INVALID_BLEND_SPACE) return INVALID_BLEND_SPACE;

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, idle_state);
    sprite_blend_space_add_point(space, idle_threshold, 0.0f, 0.0f, walk_state);
    sprite_blend_space_add_point(space, run_threshold, 0.0f, 0.0f, run_state);

    return space;
}

sprite_blend_space_t sprite_blend_space_create_2d_movement(sprite_anim_tree_t tree,
                                                            const char *forward_state,
                                                            const char *backward_state,
                                                            const char *left_state,
                                                            const char *right_state,
                                                            const char *idle_state)
{
    sprite_blend_space_t space = sprite_blend_space_create(tree, 2);
    if (space == INVALID_BLEND_SPACE) return INVALID_BLEND_SPACE;

    sprite_blend_space_add_point(space, 0.0f, 0.0f, 0.0f, idle_state);
    sprite_blend_space_add_point(space, 1.0f, 0.0f, 0.0f, forward_state);
    sprite_blend_space_add_point(space, 1.0f, 180.0f, 0.0f, backward_state);
    sprite_blend_space_add_point(space, 1.0f, 270.0f, 0.0f, left_state);
    sprite_blend_space_add_point(space, 1.0f, 90.0f, 0.0f, right_state);

    return space;
}

/* ============================================================================
 * QUERYING & INSPECTION
 * ========================================================================== */

sprite_blend_point_t sprite_blend_space_get_nearest_point(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s || s->point_count == 0) return INVALID_BLEND_POINT;

    float min_dist = 1e9f;
    int min_idx = 0;

    for (int i = 0; i < s->point_count; i++) {
        float dist = sprite_blend_space_distance_to_point(space, (sprite_blend_point_t)(intptr_t)i);
        if (dist < min_dist) {
            min_dist = dist;
            min_idx = i;
        }
    }

    return (sprite_blend_point_t)(intptr_t)min_idx;
}

void sprite_blend_space_print_state(sprite_blend_space_t space)
{
    sprite_blend_space_info_t info;
    if (!sprite_blend_space_get_info(space, &info)) {
        printf("Blend space: invalid\n");
        return;
    }

    printf("Blend Space: dims=%d points=%d mode=%d params=(%.2f,%.2f,%.2f)\n",
           info.dimension_count, info.point_count, info.mode,
           info.param_x, info.param_y, info.param_z);
}

void sprite_blend_space_print_points(sprite_blend_space_t space)
{
    sprite_blend_space_data_t *s = blend_space_get(space);
    if (!s) {
        printf("Blend space: invalid\n");
        return;
    }

    printf("Blend Space Points:\n");
    for (int i = 0; i < s->point_count; i++) {
        blend_point_t *p = &s->points[i];
        printf("  Point %d: state=%s pos=(%.2f,%.2f,%.2f) weight=%.3f\n",
               i, p->state_name, p->x, p->y, p->z, s->weights[i]);
    }
}
