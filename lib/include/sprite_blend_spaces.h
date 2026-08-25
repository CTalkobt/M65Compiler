/* sprite_blend_spaces.h — 2D Animation Blend Spaces for MEGA65
 *
 * Provides parameter-based animation blending for procedural animation control.
 * Enables smooth transitions between animations based on continuous parameters.
 *
 * Features:
 * - 2D parameter space (e.g., speed vs direction)
 * - Animation point blending at parameter coordinates
 * - Automatic weight calculation for surrounding animations
 * - Support for linear and radial blending
 * - Smooth parameter interpolation
 *
 * Usage:
 *   #include <sprite_blend_spaces.h>
 *   sprite_blend_space_t space = sprite_blend_space_create(tree, 2);
 *   // X axis: speed (0-10), Y axis: direction (0-360)
 *   sprite_blend_space_add_point(space, 0, 0, "idle");
 *   sprite_blend_space_add_point(space, 5, 0, "run_forward");
 *   sprite_blend_space_add_point(space, 5, 180, "run_backward");
 *   sprite_blend_space_set_parameters(space, 7.5f, 45.0f);  // Interpolates between points
 */

#ifndef SPRITE_BLEND_SPACES_H
#define SPRITE_BLEND_SPACES_H

#include <sprite_animation_trees.h>

/* ============================================================================
 * BLEND SPACE TYPES
 * ========================================================================== */

typedef void *sprite_blend_space_t;      /* Blend space handle */
typedef void *sprite_blend_point_t;      /* Blend point handle */

#define INVALID_BLEND_SPACE  NULL
#define INVALID_BLEND_POINT  NULL

/* ============================================================================
 * BLEND SPACE STRUCTURES
 * ========================================================================== */

typedef enum {
    BLEND_MODE_LINEAR = 0,      /* Linear interpolation */
    BLEND_MODE_RADIAL = 1,      /* Radial/circular interpolation */
    BLEND_MODE_CARTESIAN = 2,   /* 2D Cartesian grid */
} sprite_blend_mode_t;

typedef struct {
    float x, y;                 /* Parameter coordinates */
    char state_name[64];        /* Animation state name */
} sprite_blend_point_info_t;

typedef struct {
    sprite_blend_space_t space;
    int dimension_count;        /* 1D, 2D, or 3D */
    int point_count;            /* Number of blend points */
    float param_x, param_y, param_z;  /* Current parameter values */
    sprite_blend_mode_t mode;   /* Blending mode */
    int dirty;                  /* Parameters changed flag */
} sprite_blend_space_info_t;

/* ============================================================================
 * BLEND SPACE CREATION & CONTROL
 * ========================================================================== */

/**
 * sprite_blend_space_create - Create a new blend space
 *
 * Creates a parameter-based animation space for procedural blending.
 *
 * Parameters:
 *   tree — Animation tree to bind to
 *   dimensions — Number of blend dimensions (1, 2, or 3)
 *
 * Returns:
 *   Blend space handle on success, INVALID_BLEND_SPACE on error
 */
sprite_blend_space_t sprite_blend_space_create(sprite_anim_tree_t tree,
                                                int dimensions);

/**
 * sprite_blend_space_destroy - Destroy blend space
 *
 * Parameters:
 *   space — Blend space handle to destroy
 */
void sprite_blend_space_destroy(sprite_blend_space_t space);

/**
 * sprite_blend_space_get_info - Get blend space information
 *
 * Parameters:
 *   space — Blend space handle
 *   info — Pointer to sprite_blend_space_info_t to receive data
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_blend_space_get_info(sprite_blend_space_t space,
                                 sprite_blend_space_info_t *info);

/**
 * sprite_blend_space_set_mode - Set blending interpolation mode
 *
 * Parameters:
 *   space — Blend space handle
 *   mode — Blending mode (linear, radial, cartesian)
 */
void sprite_blend_space_set_mode(sprite_blend_space_t space,
                                  sprite_blend_mode_t mode);

/**
 * sprite_blend_space_get_mode - Get current blending mode
 *
 * Returns:
 *   Current blend mode
 */
sprite_blend_mode_t sprite_blend_space_get_mode(sprite_blend_space_t space);

/* ============================================================================
 * BLEND POINT MANAGEMENT
 * ========================================================================== */

/**
 * sprite_blend_space_add_point - Add animation point to space
 *
 * Adds an animation state at a specific parameter coordinate.
 * Multiple points enable interpolation between animations.
 *
 * Parameters:
 *   space — Blend space handle
 *   x, y, z — Parameter coordinates (z optional for 3D spaces)
 *   state_name — Animation state to play at this point
 *
 * Returns:
 *   Point handle on success, INVALID_BLEND_POINT on error
 */
sprite_blend_point_t sprite_blend_space_add_point(sprite_blend_space_t space,
                                                   float x, float y, float z,
                                                   const char *state_name);

/**
 * sprite_blend_space_remove_point - Remove point from space
 *
 * Parameters:
 *   space — Blend space handle
 *   point — Point handle to remove
 *
 * Returns:
 *   1 if successful, 0 if point not found
 */
int sprite_blend_space_remove_point(sprite_blend_space_t space,
                                     sprite_blend_point_t point);

/**
 * sprite_blend_space_get_point_count - Get number of blend points
 *
 * Parameters:
 *   space — Blend space handle
 *
 * Returns:
 *   Number of blend points
 */
int sprite_blend_space_get_point_count(sprite_blend_space_t space);

/**
 * sprite_blend_space_get_point - Get point by index
 *
 * Parameters:
 *   space — Blend space handle
 *   index — Point index (0 to count-1)
 *   info — Pointer to sprite_blend_point_info_t to receive data
 *
 * Returns:
 *   1 if successful, 0 if index out of range
 */
int sprite_blend_space_get_point(sprite_blend_space_t space, int index,
                                  sprite_blend_point_info_t *info);

/**
 * sprite_blend_space_find_point - Find point by state name
 *
 * Parameters:
 *   space — Blend space handle
 *   state_name — State name to find
 *
 * Returns:
 *   Point handle, or INVALID_BLEND_POINT if not found
 */
sprite_blend_point_t sprite_blend_space_find_point(sprite_blend_space_t space,
                                                    const char *state_name);

/**
 * sprite_blend_space_move_point - Move point to new coordinates
 *
 * Parameters:
 *   space — Blend space handle
 *   point — Point handle
 *   x, y, z — New coordinates
 *
 * Returns:
 *   1 if successful, 0 if point not found
 */
int sprite_blend_space_move_point(sprite_blend_space_t space,
                                   sprite_blend_point_t point,
                                   float x, float y, float z);

/* ============================================================================
 * PARAMETER CONTROL
 * ========================================================================== */

/**
 * sprite_blend_space_set_parameters - Set blend space parameters
 *
 * Sets the current position in parameter space.
 * Animation blends automatically between nearby points.
 *
 * Parameters:
 *   space — Blend space handle
 *   x, y, z — Parameter values (z optional for 2D spaces)
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_blend_space_set_parameters(sprite_blend_space_t space,
                                       float x, float y, float z);

/**
 * sprite_blend_space_get_parameters - Get current parameters
 *
 * Parameters:
 *   space — Blend space handle
 *   x, y, z — Pointers to receive parameter values
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_blend_space_get_parameters(sprite_blend_space_t space,
                                       float *x, float *y, float *z);

/**
 * sprite_blend_space_set_parameter_x - Set X parameter
 *
 * Parameters:
 *   space — Blend space handle
 *   value — X parameter value
 */
void sprite_blend_space_set_parameter_x(sprite_blend_space_t space, float value);

/**
 * sprite_blend_space_set_parameter_y - Set Y parameter
 *
 * Parameters:
 *   space — Blend space handle
 *   value — Y parameter value
 */
void sprite_blend_space_set_parameter_y(sprite_blend_space_t space, float value);

/**
 * sprite_blend_space_set_parameter_z - Set Z parameter
 *
 * Parameters:
 *   space — Blend space handle
 *   value — Z parameter value
 */
void sprite_blend_space_set_parameter_z(sprite_blend_space_t space, float value);

/**
 * sprite_blend_space_get_parameter_x - Get X parameter
 *
 * Returns:
 *   Current X value
 */
float sprite_blend_space_get_parameter_x(sprite_blend_space_t space);

/**
 * sprite_blend_space_get_parameter_y - Get Y parameter
 *
 * Returns:
 *   Current Y value
 */
float sprite_blend_space_get_parameter_y(sprite_blend_space_t space);

/**
 * sprite_blend_space_get_parameter_z - Get Z parameter
 *
 * Returns:
 *   Current Z value
 */
float sprite_blend_space_get_parameter_z(sprite_blend_space_t space);

/* ============================================================================
 * BLENDING & INTERPOLATION
 * ========================================================================== */

/**
 * sprite_blend_space_update - Update animation based on current parameters
 *
 * Recalculates blend weights and updates active animations.
 * Should be called after changing parameters.
 *
 * Parameters:
 *   space — Blend space handle
 *   delta_ms — Elapsed time in milliseconds
 *
 * Returns:
 *   Number of animations updated
 */
int sprite_blend_space_update(sprite_blend_space_t space, int delta_ms);

/**
 * sprite_blend_space_get_active_state - Get currently blending state
 *
 * Returns the primary animation state being played.
 * Other states may blend in with reduced weight.
 *
 * Parameters:
 *   space — Blend space handle
 *
 * Returns:
 *   State name (allocated string, caller must free), or NULL if none active
 */
char *sprite_blend_space_get_active_state(sprite_blend_space_t space);

/**
 * sprite_blend_space_get_blend_weights - Get current blend weights
 *
 * Returns array of blend weights for each point.
 * Weights sum to 1.0 across active points.
 *
 * Parameters:
 *   space — Blend space handle
 *   weights — Array to receive weights (pre-allocated)
 *   max_weights — Size of weights array
 *
 * Returns:
 *   Number of weights returned
 */
int sprite_blend_space_get_blend_weights(sprite_blend_space_t space,
                                          float *weights, int max_weights);

/**
 * sprite_blend_space_distance_to_point - Calculate distance to blend point
 *
 * Useful for determining which points influence current parameter position.
 *
 * Parameters:
 *   space — Blend space handle
 *   point — Point handle
 *
 * Returns:
 *   Euclidean distance in parameter space
 */
float sprite_blend_space_distance_to_point(sprite_blend_space_t space,
                                            sprite_blend_point_t point);

/* ============================================================================
 * COMMON BLEND SPACE PATTERNS
 * ========================================================================== */

/**
 * sprite_blend_space_create_1d_speed - Create 1D speed blend space
 *
 * Convenience function for speed-based animation (idle → walk → run).
 *
 * Parameters:
 *   tree — Animation tree
 *   idle_threshold — Speed to start walking
 *   run_threshold — Speed to start running
 *   idle_state — Animation for idle
 *   walk_state — Animation for walking
 *   run_state — Animation for running
 *
 * Returns:
 *   Configured blend space handle
 */
sprite_blend_space_t sprite_blend_space_create_1d_speed(sprite_anim_tree_t tree,
                                                         float idle_threshold,
                                                         float run_threshold,
                                                         const char *idle_state,
                                                         const char *walk_state,
                                                         const char *run_state);

/**
 * sprite_blend_space_create_2d_movement - Create 2D movement blend space
 *
 * Convenience function for speed+direction animation (strafe, run forward/back).
 *
 * Parameters:
 *   tree — Animation tree
 *   forward_state — Forward movement animation
 *   backward_state — Backward movement animation
 *   left_state — Left strafe animation
 *   right_state — Right strafe animation
 *   idle_state — Idle animation
 *
 * Returns:
 *   Configured blend space handle
 */
sprite_blend_space_t sprite_blend_space_create_2d_movement(sprite_anim_tree_t tree,
                                                            const char *forward_state,
                                                            const char *backward_state,
                                                            const char *left_state,
                                                            const char *right_state,
                                                            const char *idle_state);

/* ============================================================================
 * QUERYING & INSPECTION
 * ========================================================================== */

/**
 * sprite_blend_space_get_nearest_point - Find closest blend point
 *
 * Useful for determining which animation is dominant at current parameters.
 *
 * Parameters:
 *   space — Blend space handle
 *
 * Returns:
 *   Handle to nearest point, or INVALID_BLEND_POINT if no points
 */
sprite_blend_point_t sprite_blend_space_get_nearest_point(sprite_blend_space_t space);

/**
 * sprite_blend_space_print_state - Print blend space state (debug)
 *
 * Parameters:
 *   space — Blend space handle
 */
void sprite_blend_space_print_state(sprite_blend_space_t space);

/**
 * sprite_blend_space_print_points - Print all blend points (debug)
 *
 * Parameters:
 *   space — Blend space handle
 */
void sprite_blend_space_print_points(sprite_blend_space_t space);

#endif
