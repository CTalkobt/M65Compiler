/* sprite_transform.h — Sprite 2D Transforms for MEGA65
 *
 * Provides scaling, rotation, and skew transforms for sprites.
 * Extends Phase 34 sprites with affine 2D transformations.
 *
 * Features:
 * - 2D scale (independent X, Y)
 * - Rotation (0-360 degrees)
 * - Skew/shear (X, Y axis skew)
 * - Transform origin (rotation/scale center point)
 * - Matrix composition and interpolation
 * - Inverse transforms (screen coordinates to local)
 *
 * Usage:
 *   #include <sprite_transform.h>
 *   sprite_t sprite = sprite_create(100, 100, 32, 32);
 *   sprite_scale(sprite, 1.5f, 1.5f);        // 1.5x larger
 *   sprite_rotate(sprite, 45.0f);            // 45 degree rotation
 *   sprite_set_transform_origin(sprite, 16, 16);  // Center point
 */

#ifndef SPRITE_TRANSFORM_H
#define SPRITE_TRANSFORM_H

#include <sprites.h>

/* ============================================================================
 * TRANSFORM MATRIX STRUCTURE
 * ========================================================================== */

typedef struct {
    float a, b;     /* Row 0: [a, b] for X basis vector */
    float c, d;     /* Row 1: [c, d] for Y basis vector */
    float tx, ty;   /* Translation component */
} sprite_matrix_t;

/* ============================================================================
 * TRANSFORM STATE STRUCTURE
 * ========================================================================== */

typedef struct {
    float scale_x, scale_y;         /* Scale factors (1.0 = normal) */
    float rotation;                 /* Rotation in degrees (0-360) */
    float skew_x, skew_y;           /* Skew in degrees */
    int origin_x, origin_y;         /* Transform origin (local coordinates) */
    sprite_matrix_t matrix;         /* Computed transform matrix */
    int dirty;                      /* Matrix needs recomputation */
} sprite_transform_t;

/* ============================================================================
 * TRANSFORM CONTROL
 * ========================================================================== */

/**
 * sprite_scale - Set sprite scale
 *
 * Scales sprite uniformly or non-uniformly.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   scale_x, scale_y — Scale factors (1.0 = normal, 2.0 = 2x, 0.5 = half)
 */
void sprite_scale(sprite_t sprite, float scale_x, float scale_y);

/**
 * sprite_scale_uniform - Set uniform scale
 *
 * Parameters:
 *   sprite — Sprite handle
 *   scale — Scale factor (1.0 = normal)
 */
void sprite_scale_uniform(sprite_t sprite, float scale);

/**
 * sprite_get_scale - Get current scale
 *
 * Parameters:
 *   sprite — Sprite handle
 *   scale_x, scale_y — Pointers to receive scale factors
 */
void sprite_get_scale(sprite_t sprite, float *scale_x, float *scale_y);

/**
 * sprite_rotate - Set sprite rotation
 *
 * Rotates sprite around its transform origin.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   angle_degrees — Rotation angle (0-360 degrees)
 *                   0° = right, 90° = down, 180° = left, 270° = up
 */
void sprite_rotate(sprite_t sprite, float angle_degrees);

/**
 * sprite_get_rotation - Get current rotation
 *
 * Returns:
 *   Rotation angle in degrees (0-360)
 */
float sprite_get_rotation(sprite_t sprite);

/**
 * sprite_rotate_relative - Rotate by relative amount
 *
 * Parameters:
 *   sprite — Sprite handle
 *   delta_degrees — Relative rotation
 */
void sprite_rotate_relative(sprite_t sprite, float delta_degrees);

/**
 * sprite_skew - Set sprite skew (shear)
 *
 * Parameters:
 *   sprite — Sprite handle
 *   skew_x, skew_y — Skew angles in degrees
 */
void sprite_skew(sprite_t sprite, float skew_x, float skew_y);

/**
 * sprite_get_skew - Get current skew
 *
 * Parameters:
 *   sprite — Sprite handle
 *   skew_x, skew_y — Pointers to receive skew angles
 */
void sprite_get_skew(sprite_t sprite, float *skew_x, float *skew_y);

/**
 * sprite_set_transform_origin - Set rotation/scale center point
 *
 * By default, transforms originate from (0, 0) in sprite coordinates.
 * Set to (width/2, height/2) for center-based rotation.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Origin point (local coordinates)
 */
void sprite_set_transform_origin(sprite_t sprite, int x, int y);

/**
 * sprite_get_transform_origin - Get transform origin
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Pointers to receive origin
 */
void sprite_get_transform_origin(sprite_t sprite, int *x, int *y);

/**
 * sprite_set_transform_origin_centered - Set origin to sprite center
 *
 * Convenience function for center-based rotation/scaling.
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_set_transform_origin_centered(sprite_t sprite);

/* ============================================================================
 * MATRIX OPERATIONS
 * ========================================================================== */

/**
 * sprite_get_transform_matrix - Get current transform matrix
 *
 * Returns the computed 2D affine transformation matrix.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   matrix — Pointer to sprite_matrix_t to receive matrix
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_get_transform_matrix(sprite_t sprite, sprite_matrix_t *matrix);

/**
 * sprite_set_transform_matrix - Set transform from matrix
 *
 * Directly set transform using a matrix. Decomposes back to
 * scale/rotation/skew for consistency.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   matrix — Transformation matrix
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_set_transform_matrix(sprite_t sprite, const sprite_matrix_t *matrix);

/**
 * sprite_reset_transform - Reset to identity (no transform)
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_reset_transform(sprite_t sprite);

/**
 * sprite_compose_transforms - Compose two transforms
 *
 * Multiplies two matrices: result = a * b
 *
 * Parameters:
 *   a, b — Input matrices
 *   result — Pointer to receive result matrix
 */
void sprite_compose_transforms(const sprite_matrix_t *a,
                               const sprite_matrix_t *b,
                               sprite_matrix_t *result);

/**
 * sprite_invert_transform - Compute inverse transform
 *
 * Useful for converting screen coordinates to local sprite space.
 *
 * Parameters:
 *   matrix — Input matrix
 *   inverse — Pointer to receive inverse matrix
 *
 * Returns:
 *   1 if invertible, 0 if singular (determinant ≈ 0)
 */
int sprite_invert_transform(const sprite_matrix_t *matrix,
                            sprite_matrix_t *inverse);

/* ============================================================================
 * COORDINATE TRANSFORMATION
 * ========================================================================== */

/**
 * sprite_transform_point - Transform point using sprite's matrix
 *
 * Converts local coordinates to world coordinates.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   local_x, local_y — Local coordinates
 *   world_x, world_y — Pointers to receive world coordinates
 */
void sprite_transform_point(sprite_t sprite, float local_x, float local_y,
                            float *world_x, float *world_y);

/**
 * sprite_inverse_transform_point - Transform point using inverse matrix
 *
 * Converts world coordinates to local coordinates (useful for hit-testing).
 *
 * Parameters:
 *   sprite — Sprite handle
 *   world_x, world_y — World coordinates
 *   local_x, local_y — Pointers to receive local coordinates
 *
 * Returns:
 *   1 if successful, 0 if transform is singular
 */
int sprite_inverse_transform_point(sprite_t sprite, float world_x, float world_y,
                                   float *local_x, float *local_y);

/* ============================================================================
 * INTERPOLATION & ANIMATION
 * ========================================================================== */

/**
 * sprite_lerp_transform - Linear interpolate between transforms
 *
 * Smoothly blends from source to target transform.
 *
 * Parameters:
 *   src — Source transform
 *   dst — Destination transform
 *   t — Interpolation factor (0.0 = src, 1.0 = dst)
 *   result — Pointer to receive interpolated transform
 */
void sprite_lerp_transform(const sprite_transform_t *src,
                           const sprite_transform_t *dst,
                           float t,
                           sprite_transform_t *result);

/**
 * sprite_animate_transform - Animate from current to target transform
 *
 * Returns true if animation in progress, false if complete.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   target_scale_x, target_scale_y — Target scale
 *   target_rotation — Target rotation
 *   speed — Animation speed (0.1-1.0, where 1.0 = full per frame)
 *
 * Returns:
 *   1 if still animating, 0 if complete
 */
int sprite_animate_transform(sprite_t sprite, float target_scale_x,
                             float target_scale_y, float target_rotation,
                             float speed);

/* ============================================================================
 * COLLISION WITH TRANSFORMS
 * ========================================================================== */

/**
 * sprite_collides_point_transformed - Hit test with transform applied
 *
 * Tests if point is within sprite's bounding box after transformation.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Screen coordinates to test
 *
 * Returns:
 *   1 if point hits sprite, 0 otherwise
 */
int sprite_collides_point_transformed(sprite_t sprite, int x, int y);

/**
 * sprite_get_bounds_transformed - Get transformed bounding box
 *
 * Returns the axis-aligned bounding box after applying transforms.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x1, y1, x2, y2 — Pointers to receive bounds
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_get_bounds_transformed(sprite_t sprite, int *x1, int *y1,
                                  int *x2, int *y2);

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * sprite_print_transform - Print transform state (debug)
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_print_transform(sprite_t sprite);

/**
 * sprite_print_matrix - Print transform matrix (debug)
 *
 * Parameters:
 *   matrix — Matrix to print
 */
void sprite_print_matrix(const sprite_matrix_t *matrix);

#endif
