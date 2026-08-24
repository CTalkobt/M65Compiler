/* sprite_groups.c — Sprite Groups & Hierarchies Implementation
 *
 * Provides sprite grouping with parent-child relationships and hierarchical transforms.
 */

#include <sprite_groups.h>
#include <sprite_transform.h>
#include <sprite_dynamics.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_GROUPS 128
#define MAX_MEMBERS_PER_GROUP 256
#define MAX_CHILDREN_PER_SPRITE 32

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ========================================================================== */

typedef struct {
    sprite_t *members;
    int member_count;
    int member_capacity;
    int x, y;
    int visible;
    int layer;
    float scale_x, scale_y;
    float rotation;
    int origin_x, origin_y;
    int dirty;
} sprite_group_data_t;

typedef struct {
    sprite_t parent;
    sprite_t *children;
    int child_count;
    int child_capacity;
} sprite_hierarchy_t;

static sprite_group_data_t g_groups[MAX_GROUPS];
static int g_group_count = 0;
static sprite_hierarchy_t g_hierarchy[MAX_GROUPS * MAX_MEMBERS_PER_GROUP];

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

static int sprite_group_index(sprite_group_t group)
{
    if (!group) return -1;
    int idx = (intptr_t)group;
    return (idx >= 0 && idx < g_group_count) ? idx : -1;
}

static int sprite_hierarchy_index(sprite_t sprite)
{
    if (!sprite) return -1;
    int idx = (intptr_t)sprite;
    return idx;
}

static void sprite_hierarchy_init(sprite_t sprite)
{
    int idx = sprite_hierarchy_index(sprite);
    if (idx >= 0 && idx < (int)sizeof(g_hierarchy)/sizeof(g_hierarchy[0])) {
        if (g_hierarchy[idx].children == NULL) {
            g_hierarchy[idx].children = malloc(MAX_CHILDREN_PER_SPRITE * sizeof(sprite_t));
            g_hierarchy[idx].parent = NULL;
            g_hierarchy[idx].child_count = 0;
            g_hierarchy[idx].child_capacity = MAX_CHILDREN_PER_SPRITE;
        }
    }
}

/* ============================================================================
 * GROUP CREATION & DESTRUCTION
 * ========================================================================== */

sprite_group_t sprite_group_create(int x, int y)
{
    if (g_group_count >= MAX_GROUPS) return INVALID_GROUP;

    int idx = g_group_count;
    sprite_group_data_t *group = &g_groups[idx];

    group->members = malloc(MAX_MEMBERS_PER_GROUP * sizeof(sprite_t));
    if (!group->members) return INVALID_GROUP;

    group->member_count = 0;
    group->member_capacity = MAX_MEMBERS_PER_GROUP;
    group->x = x;
    group->y = y;
    group->visible = 1;
    group->layer = 0;
    group->scale_x = 1.0f;
    group->scale_y = 1.0f;
    group->rotation = 0.0f;
    group->origin_x = 0;
    group->origin_y = 0;
    group->dirty = 0;

    g_group_count++;
    return (sprite_group_t)(intptr_t)idx;
}

void sprite_group_destroy(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;

    sprite_group_data_t *g = &g_groups[idx];
    if (g->members) {
        free(g->members);
        g->members = NULL;
    }
    g->member_count = 0;
}

int sprite_group_get_info(sprite_group_t group, sprite_group_info_t *info)
{
    if (!info) return 0;
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    info->id = group;
    info->x = g->x;
    info->y = g->y;
    info->visible = g->visible;
    info->layer = g->layer;
    info->member_count = g->member_count;
    info->is_transform_group = (g->scale_x != 1.0f || g->scale_y != 1.0f ||
                                g->rotation != 0.0f);
    return 1;
}

/* ============================================================================
 * GROUP MEMBERSHIP
 * ========================================================================== */

int sprite_group_add_sprite(sprite_group_t group, sprite_t sprite)
{
    if (!sprite) return 0;
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    if (g->member_count >= g->member_capacity) return 0;

    g->members[g->member_count++] = sprite;
    sprite_hierarchy_init(sprite);
    return 1;
}

int sprite_group_remove_sprite(sprite_group_t group, sprite_t sprite)
{
    if (!sprite) return 0;
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    for (int i = 0; i < g->member_count; i++) {
        if (g->members[i] == sprite) {
            for (int j = i; j < g->member_count - 1; j++) {
                g->members[j] = g->members[j + 1];
            }
            g->member_count--;
            return 1;
        }
    }
    return 0;
}

int sprite_group_clear(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    int count = g->member_count;
    g->member_count = 0;
    return count;
}

int sprite_group_get_member_count(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    return (idx < 0) ? 0 : g_groups[idx].member_count;
}

sprite_t sprite_group_get_member(sprite_group_t group, int index)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return INVALID_SPRITE;

    sprite_group_data_t *g = &g_groups[idx];
    if (index < 0 || index >= g->member_count) return INVALID_SPRITE;
    return g->members[index];
}

int sprite_is_in_group(sprite_group_t group, sprite_t sprite)
{
    if (!sprite) return 0;
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    for (int i = 0; i < g->member_count; i++) {
        if (g->members[i] == sprite) return 1;
    }
    return 0;
}

/* ============================================================================
 * GROUP POSITIONING
 * ========================================================================== */

void sprite_group_set_position(sprite_group_t group, int x, int y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].x = x;
    g_groups[idx].y = y;
}

void sprite_group_move(sprite_group_t group, int dx, int dy)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].x += dx;
    g_groups[idx].y += dy;
}

int sprite_group_get_position(sprite_group_t group, int *x, int *y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;
    if (x) *x = g_groups[idx].x;
    if (y) *y = g_groups[idx].y;
    return 1;
}

int sprite_group_get_bounds(sprite_group_t group, int *x1, int *y1,
                            int *x2, int *y2)
{
    if (!x1 || !y1 || !x2 || !y2) return 0;
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    if (g->member_count == 0) return 0;

    int min_x = 1e6, max_x = -1e6;
    int min_y = 1e6, max_y = -1e6;

    for (int i = 0; i < g->member_count; i++) {
        int sx1, sy1, sx2, sy2;
        if (sprite_get_bounds(g->members[i], &sx1, &sy1, &sx2, &sy2)) {
            if (sx1 < min_x) min_x = sx1;
            if (sx2 > max_x) max_x = sx2;
            if (sy1 < min_y) min_y = sy1;
            if (sy2 > max_y) max_y = sy2;
        }
    }

    *x1 = min_x;
    *y1 = min_y;
    *x2 = max_x;
    *y2 = max_y;
    return 1;
}

/* ============================================================================
 * GROUP VISIBILITY & ATTRIBUTES
 * ========================================================================== */

void sprite_group_set_visible(sprite_group_t group, int visible)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].visible = visible ? 1 : 0;
}

int sprite_group_is_visible(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    return (idx < 0) ? 0 : g_groups[idx].visible;
}

void sprite_group_set_layer(sprite_group_t group, int layer)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].layer = layer;
}

int sprite_group_get_layer(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    return (idx < 0) ? 0 : g_groups[idx].layer;
}

/* ============================================================================
 * GROUP TRANSFORMS
 * ========================================================================== */

void sprite_group_scale(sprite_group_t group, float scale_x, float scale_y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].scale_x = scale_x;
    g_groups[idx].scale_y = scale_y;
    g_groups[idx].dirty = 1;
}

void sprite_group_scale_uniform(sprite_group_t group, float scale)
{
    sprite_group_scale(group, scale, scale);
}

int sprite_group_get_scale(sprite_group_t group, float *scale_x,
                           float *scale_y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;
    if (scale_x) *scale_x = g_groups[idx].scale_x;
    if (scale_y) *scale_y = g_groups[idx].scale_y;
    return 1;
}

void sprite_group_rotate(sprite_group_t group, float angle_degrees)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    while (angle_degrees < 0.0f) angle_degrees += 360.0f;
    while (angle_degrees >= 360.0f) angle_degrees -= 360.0f;
    g_groups[idx].rotation = angle_degrees;
    g_groups[idx].dirty = 1;
}

float sprite_group_get_rotation(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    return (idx < 0) ? 0.0f : g_groups[idx].rotation;
}

void sprite_group_rotate_relative(sprite_group_t group, float delta_degrees)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    sprite_group_rotate(group, g_groups[idx].rotation + delta_degrees);
}

void sprite_group_reset_transform(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    sprite_group_data_t *g = &g_groups[idx];
    g->scale_x = 1.0f;
    g->scale_y = 1.0f;
    g->rotation = 0.0f;
    g->origin_x = 0;
    g->origin_y = 0;
    g->dirty = 1;
}

void sprite_group_set_transform_origin(sprite_group_t group, int x, int y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return;
    g_groups[idx].origin_x = x;
    g_groups[idx].origin_y = y;
    g_groups[idx].dirty = 1;
}

void sprite_group_get_transform_origin(sprite_group_t group, int *x, int *y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) {
        if (x) *x = 0;
        if (y) *y = 0;
        return;
    }
    if (x) *x = g_groups[idx].origin_x;
    if (y) *y = g_groups[idx].origin_y;
}

/* ============================================================================
 * PARENT-CHILD RELATIONSHIPS
 * ========================================================================== */

int sprite_set_parent(sprite_t sprite, sprite_t parent)
{
    if (!sprite) return 0;

    sprite_hierarchy_init(sprite);
    int sprite_idx = sprite_hierarchy_index(sprite);
    if (sprite_idx < 0) return 0;

    sprite_hierarchy_t *h = &g_hierarchy[sprite_idx];

    if (h->parent) {
        int parent_idx = sprite_hierarchy_index(h->parent);
        if (parent_idx >= 0) {
            sprite_hierarchy_t *ph = &g_hierarchy[parent_idx];
            for (int i = 0; i < ph->child_count; i++) {
                if (ph->children[i] == sprite) {
                    for (int j = i; j < ph->child_count - 1; j++) {
                        ph->children[j] = ph->children[j + 1];
                    }
                    ph->child_count--;
                    break;
                }
            }
        }
    }

    h->parent = parent;

    if (parent) {
        sprite_hierarchy_init(parent);
        int parent_idx = sprite_hierarchy_index(parent);
        if (parent_idx >= 0) {
            sprite_hierarchy_t *ph = &g_hierarchy[parent_idx];
            if (ph->child_count < ph->child_capacity) {
                ph->children[ph->child_count++] = sprite;
            }
        }
    }

    return 1;
}

sprite_t sprite_get_parent(sprite_t sprite)
{
    if (!sprite) return INVALID_SPRITE;
    int idx = sprite_hierarchy_index(sprite);
    if (idx < 0) return INVALID_SPRITE;
    return g_hierarchy[idx].parent;
}

int sprite_get_child_count(sprite_t sprite)
{
    if (!sprite) return 0;
    int idx = sprite_hierarchy_index(sprite);
    if (idx < 0) return 0;
    return g_hierarchy[idx].child_count;
}

sprite_t sprite_get_child(sprite_t sprite, int index)
{
    if (!sprite) return INVALID_SPRITE;
    int idx = sprite_hierarchy_index(sprite);
    if (idx < 0) return INVALID_SPRITE;

    sprite_hierarchy_t *h = &g_hierarchy[idx];
    if (index < 0 || index >= h->child_count) return INVALID_SPRITE;
    return h->children[index];
}

int sprite_is_child_of(sprite_t child, sprite_t parent)
{
    if (!child || !parent) return 0;
    return sprite_get_parent(child) == parent;
}

int sprite_is_ancestor_of(sprite_t ancestor, sprite_t descendant)
{
    if (!ancestor || !descendant) return 0;

    sprite_t current = sprite_get_parent(descendant);
    while (current) {
        if (current == ancestor) return 1;
        current = sprite_get_parent(current);
    }
    return 0;
}

/* ============================================================================
 * BATCH OPERATIONS
 * ========================================================================== */

int sprite_group_update_all(sprite_group_t group, int delta_ms)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    int updated = 0;

    for (int i = 0; i < g->member_count; i++) {
        if (sprite_update_single(g->members[i], delta_ms)) {
            updated++;
        }
    }

    return updated;
}

int sprite_group_render_all(sprite_group_t group)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    if (!g->visible) return 0;

    int rendered = 0;
    for (int i = 0; i < g->member_count; i++) {
        if (sprite_is_visible(g->members[i])) {
            rendered++;
        }
    }

    return rendered;
}

int sprite_group_collides_point(sprite_group_t group, int x, int y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return 0;

    sprite_group_data_t *g = &g_groups[idx];
    for (int i = 0; i < g->member_count; i++) {
        if (sprite_collides_point_transformed(g->members[i], x, y)) {
            return 1;
        }
    }
    return 0;
}

sprite_t sprite_group_collides_point_get(sprite_group_t group, int x, int y)
{
    int idx = sprite_group_index(group);
    if (idx < 0) return INVALID_SPRITE;

    sprite_group_data_t *g = &g_groups[idx];
    sprite_t topmost = INVALID_SPRITE;
    int top_layer = -1;

    for (int i = 0; i < g->member_count; i++) {
        if (sprite_collides_point_transformed(g->members[i], x, y)) {
            int layer = sprite_get_layer(g->members[i]);
            if (layer > top_layer) {
                topmost = g->members[i];
                top_layer = layer;
            }
        }
    }

    return topmost;
}

/* ============================================================================
 * GROUP ENUMERATION
 * ========================================================================== */

int sprite_group_enumerate(sprite_group_callback callback)
{
    for (int i = 0; i < g_group_count; i++) {
        if (callback) {
            callback((sprite_group_t)(intptr_t)i);
        }
    }
    return g_group_count;
}

int sprite_count_all_groups(void)
{
    return g_group_count;
}

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

void sprite_print_group(sprite_group_t group)
{
    sprite_group_info_t info;
    if (!sprite_group_get_info(group, &info)) {
        printf("Group: invalid\n");
        return;
    }

    printf("Group: pos=(%d,%d) visible=%d layer=%d members=%d transform=%d\n",
           info.x, info.y, info.visible, info.layer, info.member_count,
           info.is_transform_group);
}

void sprite_print_hierarchy(sprite_t sprite, int indent)
{
    if (!sprite) return;

    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) return;

    for (int i = 0; i < indent; i++) printf("  ");
    printf("Sprite: pos=(%d,%d) size=%dx%d children=%d\n",
           info.x, info.y, info.width, info.height,
           sprite_get_child_count(sprite));

    for (int i = 0; i < sprite_get_child_count(sprite); i++) {
        sprite_t child = sprite_get_child(sprite, i);
        sprite_print_hierarchy(child, indent + 1);
    }
}
