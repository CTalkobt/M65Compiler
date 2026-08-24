/* sprite_transform.c — Sprite 2D Transforms Implementation
 *
 * Provides 2D affine transformation support for sprites.
 * Implements scaling, rotation, skew, and coordinate transformation.
 */

#include <sprite_transform.h>
#include <sprites.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define PI 3.14159265358979323846f
#define DEG_TO_RAD(d) ((d) * PI / 180.0f)
#define RAD_TO_DEG(r) ((r) * 180.0f / PI)

static inline float fabs_f(float x) { return x < 0 ? -x : x; }
static inline float fmax_f(float a, float b) { return a > b ? a : b; }
static inline float fmin_f(float a, float b) { return a < b ? a : b; }

/* ============================================================================
 * INTERNAL STATE MANAGEMENT
 * ========================================================================== */

typedef struct {
    sprite_transform_t *transforms;
    int max_sprites;
} sprite_transform_system_t;

static sprite_transform_system_t g_transform_system = {0, 0};

static sprite_transform_t *sprite_get_transform_internal(sprite_t sprite)
{
    int idx = (intptr_t)sprite;
    if (idx < 0 || idx >= g_transform_system.max_sprites)
        return NULL;
    return &g_transform_system.transforms[idx];
}

static void sprite_ensure_transform_allocated(sprite_t sprite)
{
    int idx = (intptr_t)sprite;
    if (idx >= g_transform_system.max_sprites) {
        int new_size = idx + 64;
        sprite_transform_t *new_transforms = realloc(g_transform_system.transforms,
                                                       new_size * sizeof(sprite_transform_t));
        if (new_transforms) {
            memset(new_transforms + g_transform_system.max_sprites, 0,
                   (new_size - g_transform_system.max_sprites) * sizeof(sprite_transform_t));
            g_transform_system.transforms = new_transforms;
            g_transform_system.max_sprites = new_size;
        }
    }

    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (t && t->dirty == -1) {
        t->scale_x = 1.0f;
        t->scale_y = 1.0f;
        t->rotation = 0.0f;
        t->skew_x = 0.0f;
        t->skew_y = 0.0f;
        t->origin_x = 0;
        t->origin_y = 0;
        t->dirty = 1;
    }
}

/* ============================================================================
 * MATRIX COMPUTATION
 * ========================================================================== */

static void sprite_compute_transform_matrix(sprite_transform_t *t)
{
    if (!t->dirty) return;

    float rot_rad = DEG_TO_RAD(t->rotation);
    float cos_r = cosf(rot_rad);
    float sin_r = sinf(rot_rad);

    float skew_x_rad = DEG_TO_RAD(t->skew_x);
    float skew_y_rad = DEG_TO_RAD(t->skew_y);
    float tan_skew_x = tanf(skew_x_rad);
    float tan_skew_y = tanf(skew_y_rad);

    float a = t->scale_x * cos_r;
    float b = t->scale_x * sin_r;
    float c = -t->scale_y * sin_r;
    float d = t->scale_y * cos_r;

    float a_skewed = a + c * tan_skew_x;
    float b_skewed = b + d * tan_skew_x;
    float c_skewed = a * tan_skew_y + c;
    float d_skewed = b * tan_skew_y + d;

    t->matrix.a = a_skewed;
    t->matrix.b = b_skewed;
    t->matrix.c = c_skewed;
    t->matrix.d = d_skewed;

    float ox = (float)t->origin_x;
    float oy = (float)t->origin_y;
    t->matrix.tx = ox - (a_skewed * ox + c_skewed * oy);
    t->matrix.ty = oy - (b_skewed * ox + d_skewed * oy);

    t->dirty = 0;
}

/* ============================================================================
 * SCALE CONTROL
 * ========================================================================== */

void sprite_scale(sprite_t sprite, float scale_x, float scale_y)
{
    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;

    t->scale_x = scale_x;
    t->scale_y = scale_y;
    t->dirty = 1;
}

void sprite_scale_uniform(sprite_t sprite, float scale)
{
    sprite_scale(sprite, scale, scale);
}

void sprite_get_scale(sprite_t sprite, float *scale_x, float *scale_y)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        if (scale_x) *scale_x = 1.0f;
        if (scale_y) *scale_y = 1.0f;
        return;
    }
    if (scale_x) *scale_x = t->scale_x;
    if (scale_y) *scale_y = t->scale_y;
}

/* ============================================================================
 * ROTATION CONTROL
 * ========================================================================== */

void sprite_rotate(sprite_t sprite, float angle_degrees)
{
    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;

    while (angle_degrees < 0.0f) angle_degrees += 360.0f;
    while (angle_degrees >= 360.0f) angle_degrees -= 360.0f;

    t->rotation = angle_degrees;
    t->dirty = 1;
}

float sprite_get_rotation(sprite_t sprite)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return 0.0f;
    return t->rotation;
}

void sprite_rotate_relative(sprite_t sprite, float delta_degrees)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;
    sprite_rotate(sprite, t->rotation + delta_degrees);
}

/* ============================================================================
 * SKEW CONTROL
 * ========================================================================== */

void sprite_skew(sprite_t sprite, float skew_x, float skew_y)
{
    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;

    t->skew_x = skew_x;
    t->skew_y = skew_y;
    t->dirty = 1;
}

void sprite_get_skew(sprite_t sprite, float *skew_x, float *skew_y)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        if (skew_x) *skew_x = 0.0f;
        if (skew_y) *skew_y = 0.0f;
        return;
    }
    if (skew_x) *skew_x = t->skew_x;
    if (skew_y) *skew_y = t->skew_y;
}

/* ============================================================================
 * TRANSFORM ORIGIN
 * ========================================================================== */

void sprite_set_transform_origin(sprite_t sprite, int x, int y)
{
    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;

    t->origin_x = x;
    t->origin_y = y;
    t->dirty = 1;
}

void sprite_get_transform_origin(sprite_t sprite, int *x, int *y)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        if (x) *x = 0;
        if (y) *y = 0;
        return;
    }
    if (x) *x = t->origin_x;
    if (y) *y = t->origin_y;
}

void sprite_set_transform_origin_centered(sprite_t sprite)
{
    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) {
        sprite_set_transform_origin(sprite, 0, 0);
        return;
    }
    sprite_set_transform_origin(sprite, info.width / 2, info.height / 2);
}

/* ============================================================================
 * MATRIX OPERATIONS
 * ========================================================================== */

int sprite_get_transform_matrix(sprite_t sprite, sprite_matrix_t *matrix)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t || !matrix) return 0;

    sprite_compute_transform_matrix(t);
    *matrix = t->matrix;
    return 1;
}

int sprite_set_transform_matrix(sprite_t sprite, const sprite_matrix_t *matrix)
{
    if (!matrix) return 0;

    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return 0;

    t->matrix = *matrix;

    float sx = sqrtf(matrix->a * matrix->a + matrix->b * matrix->b);
    float sy = sqrtf(matrix->c * matrix->c + matrix->d * matrix->d);
    float rot = atan2f(matrix->b, matrix->a);

    t->scale_x = sx > 0.001f ? sx : 1.0f;
    t->scale_y = sy > 0.001f ? sy : 1.0f;
    t->rotation = RAD_TO_DEG(rot);
    t->skew_x = 0.0f;
    t->skew_y = 0.0f;
    t->dirty = 0;

    return 1;
}

void sprite_reset_transform(sprite_t sprite)
{
    sprite_ensure_transform_allocated(sprite);
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return;

    t->scale_x = 1.0f;
    t->scale_y = 1.0f;
    t->rotation = 0.0f;
    t->skew_x = 0.0f;
    t->skew_y = 0.0f;
    t->origin_x = 0;
    t->origin_y = 0;
    t->dirty = 1;
}

void sprite_compose_transforms(const sprite_matrix_t *a,
                               const sprite_matrix_t *b,
                               sprite_matrix_t *result)
{
    if (!a || !b || !result) return;

    result->a = a->a * b->a + a->b * b->c;
    result->b = a->a * b->b + a->b * b->d;
    result->c = a->c * b->a + a->d * b->c;
    result->d = a->c * b->b + a->d * b->d;
    result->tx = a->a * b->tx + a->b * b->ty + a->tx;
    result->ty = a->c * b->tx + a->d * b->ty + a->ty;
}

int sprite_invert_transform(const sprite_matrix_t *matrix,
                            sprite_matrix_t *inverse)
{
    if (!matrix || !inverse) return 0;

    float det = matrix->a * matrix->d - matrix->b * matrix->c;
    if (fabs_f(det) < 0.0001f) return 0;

    inverse->a = matrix->d / det;
    inverse->b = -matrix->b / det;
    inverse->c = -matrix->c / det;
    inverse->d = matrix->a / det;
    inverse->tx = (matrix->b * matrix->ty - matrix->d * matrix->tx) / det;
    inverse->ty = (matrix->c * matrix->tx - matrix->a * matrix->ty) / det;

    return 1;
}

/* ============================================================================
 * COORDINATE TRANSFORMATION
 * ========================================================================== */

void sprite_transform_point(sprite_t sprite, float local_x, float local_y,
                            float *world_x, float *world_y)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        if (world_x) *world_x = local_x;
        if (world_y) *world_y = local_y;
        return;
    }

    sprite_compute_transform_matrix(t);
    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) {
        if (world_x) *world_x = local_x;
        if (world_y) *world_y = local_y;
        return;
    }

    float wx = t->matrix.a * local_x + t->matrix.c * local_y + t->matrix.tx + info.x;
    float wy = t->matrix.b * local_x + t->matrix.d * local_y + t->matrix.ty + info.y;

    if (world_x) *world_x = wx;
    if (world_y) *world_y = wy;
}

int sprite_inverse_transform_point(sprite_t sprite, float world_x, float world_y,
                                   float *local_x, float *local_y)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        if (local_x) *local_x = world_x;
        if (local_y) *local_y = world_y;
        return 1;
    }

    sprite_compute_transform_matrix(t);

    sprite_matrix_t inv;
    if (!sprite_invert_transform(&t->matrix, &inv)) return 0;

    int sx = sprite_get_x(sprite);
    int sy = sprite_get_y(sprite);

    float wx = world_x - sx;
    float wy = world_y - sy;

    float lx = inv.a * wx + inv.c * wy + inv.tx;
    float ly = inv.b * wx + inv.d * wy + inv.ty;

    if (local_x) *local_x = lx;
    if (local_y) *local_y = ly;

    return 1;
}

/* ============================================================================
 * INTERPOLATION & ANIMATION
 * ========================================================================== */

void sprite_lerp_transform(const sprite_transform_t *src,
                           const sprite_transform_t *dst,
                           float t,
                           sprite_transform_t *result)
{
    if (!src || !dst || !result) return;

    float inv_t = 1.0f - t;

    result->scale_x = src->scale_x * inv_t + dst->scale_x * t;
    result->scale_y = src->scale_y * inv_t + dst->scale_y * t;

    float src_rot = src->rotation;
    float dst_rot = dst->rotation;

    if (dst_rot - src_rot > 180.0f)
        dst_rot -= 360.0f;
    else if (dst_rot - src_rot < -180.0f)
        dst_rot += 360.0f;

    result->rotation = src_rot * inv_t + dst_rot * t;
    while (result->rotation < 0.0f) result->rotation += 360.0f;
    while (result->rotation >= 360.0f) result->rotation -= 360.0f;

    result->skew_x = src->skew_x * inv_t + dst->skew_x * t;
    result->skew_y = src->skew_y * inv_t + dst->skew_y * t;
    result->origin_x = (int)(src->origin_x * inv_t + dst->origin_x * t);
    result->origin_y = (int)(src->origin_y * inv_t + dst->origin_y * t);
    result->dirty = 1;
}

int sprite_animate_transform(sprite_t sprite, float target_scale_x,
                             float target_scale_y, float target_rotation,
                             float speed)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) return 0;

    if (speed <= 0.0f || speed > 1.0f) speed = 0.1f;

    float sx = t->scale_x;
    float sy = t->scale_y;
    float sr = t->rotation;

    float dsx = target_scale_x - sx;
    float dsy = target_scale_y - sy;
    float dsr = target_rotation - sr;

    if (dsr > 180.0f) dsr -= 360.0f;
    if (dsr < -180.0f) dsr += 360.0f;

    t->scale_x += dsx * speed;
    t->scale_y += dsy * speed;
    t->rotation += dsr * speed;

    t->dirty = 1;

    float eps = 0.01f;
    int done = (fabs_f(dsx) < eps && fabs_f(dsy) < eps && fabs_f(dsr) < eps);

    return done ? 0 : 1;
}

/* ============================================================================
 * COLLISION WITH TRANSFORMS
 * ========================================================================== */

int sprite_collides_point_transformed(sprite_t sprite, int x, int y)
{
    int x1, y1, x2, y2;
    if (!sprite_get_bounds_transformed(sprite, &x1, &y1, &x2, &y2))
        return 0;

    return (x >= x1 && x <= x2 && y >= y1 && y <= y2);
}

int sprite_get_bounds_transformed(sprite_t sprite, int *x1, int *y1,
                                  int *x2, int *y2)
{
    if (!x1 || !y1 || !x2 || !y2) return 0;

    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) return 0;

    int w = info.width;
    int h = info.height;
    int sx = info.x;
    int sy = info.y;

    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        *x1 = sx;
        *y1 = sy;
        *x2 = sx + w - 1;
        *y2 = sy + h - 1;
        return 1;
    }

    float corners[4][2] = {
        {0, 0},
        {(float)w, 0},
        {0, (float)h},
        {(float)w, (float)h}
    };

    float min_x = 1e9f, max_x = -1e9f;
    float min_y = 1e9f, max_y = -1e9f;

    for (int i = 0; i < 4; i++) {
        float wx, wy;
        sprite_transform_point(sprite, corners[i][0], corners[i][1], &wx, &wy);
        min_x = fmin_f(min_x, wx);
        max_x = fmax_f(max_x, wx);
        min_y = fmin_f(min_y, wy);
        max_y = fmax_f(max_y, wy);
    }

    *x1 = (int)min_x;
    *y1 = (int)min_y;
    *x2 = (int)max_x;
    *y2 = (int)max_y;

    return 1;
}

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

void sprite_print_transform(sprite_t sprite)
{
    sprite_transform_t *t = sprite_get_transform_internal(sprite);
    if (!t) {
        printf("Transform: uninitialized\n");
        return;
    }

    printf("Transform: scale=(%.2f, %.2f) rot=%.1f° skew=(%.1f°, %.1f°) origin=(%d, %d)\n",
           t->scale_x, t->scale_y, t->rotation, t->skew_x, t->skew_y,
           t->origin_x, t->origin_y);
}

void sprite_print_matrix(const sprite_matrix_t *matrix)
{
    if (!matrix) {
        printf("Matrix: NULL\n");
        return;
    }

    printf("Matrix:\n");
    printf("  [%.4f  %.4f] [tx=%.2f]\n", matrix->a, matrix->b, matrix->tx);
    printf("  [%.4f  %.4f] [ty=%.2f]\n", matrix->c, matrix->d, matrix->ty);
}
