/* sprite_dynamics.h — Sprite Movement & Animation for MEGA65
 *
 * Extends Phase 34 sprites with velocity-based movement and frame-based animation.
 * Movement is updated based on elapsed time via sprite_update_interval().
 * Integrates seamlessly with existing sprite system.
 *
 * Features:
 * - Vector velocity (vx, vy) with acceleration support
 * - Heading/angle-based movement (polar coordinates)
 * - Speed and friction/deceleration
 * - Frame-based animation with sequences
 * - Callback system for animation events
 * - Interval-driven updates (independent of rendering)
 *
 * Usage:
 *   #include <sprite_dynamics.h>
 *   sprite_t sprite = sprite_create(100, 100, 32, 32);
 *   sprite_set_velocity(sprite, 2.0f, 1.5f);     // vx=2, vy=1.5
 *   sprite_update_interval(16);                   // Update at 60fps (16ms)
 *   // sprite now moves by (vx, vy) each interval
 */

#ifndef SPRITE_DYNAMICS_H
#define SPRITE_DYNAMICS_H

#include <sprites.h>

/* ============================================================================
 * MOVEMENT STRUCTURE
 * ========================================================================== */

/**
 * sprite_movement_t - Movement state for a sprite
 *
 * Stores velocity, heading, acceleration, and friction.
 */
typedef struct {
    float vx, vy;           /* Velocity in pixels per interval */
    float ax, ay;           /* Acceleration (pixels per interval²) */
    float heading;          /* Direction in degrees (0-360) */
    float speed;            /* Speed in pixels per interval */
    float friction;         /* Deceleration multiplier (0.0-1.0) */
    int active;             /* Movement enabled flag */
} sprite_movement_t;

/* ============================================================================
 * ANIMATION STRUCTURE
 * ========================================================================== */

typedef enum {
    ANIM_ONCE = 0,          /* Play once, then stop */
    ANIM_LOOP = 1,          /* Loop continuously */
    ANIM_PINGPONG = 2,      /* Play forward, then backward */
} sprite_anim_mode_t;

typedef struct {
    uint8_t **frames;       /* Array of frame bitmaps */
    int frame_count;        /* Number of frames */
    int current_frame;      /* Current frame index */
    int frame_delay;        /* Intervals per frame */
    int frame_counter;      /* Internal counter */
    int playing;            /* Animation active flag */
    sprite_anim_mode_t mode;/* Play mode (once/loop/pingpong) */
    int direction;          /* 1=forward, -1=backward (for pingpong) */
} sprite_animation_t;

/* ============================================================================
 * CALLBACK TYPES
 * ========================================================================== */

/**
 * sprite_on_move_callback - Called when sprite moves
 *
 * Parameters:
 *   sprite — Sprite handle
 *   old_x, old_y — Previous position
 *   new_x, new_y — New position
 */
typedef void (*sprite_on_move_callback)(sprite_t sprite,
                                        int old_x, int old_y,
                                        int new_x, int new_y);

/**
 * sprite_on_frame_callback - Called when animation frame changes
 *
 * Parameters:
 *   sprite — Sprite handle
 *   frame — New frame index
 */
typedef void (*sprite_on_frame_callback)(sprite_t sprite, int frame);

/* ============================================================================
 * MOVEMENT CONTROL
 * ========================================================================== */

/**
 * sprite_set_velocity - Set sprite velocity (Cartesian)
 *
 * Sets velocity in pixels per interval.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   vx, vy — Velocity components
 */
void sprite_set_velocity(sprite_t sprite, float vx, float vy);

/**
 * sprite_get_velocity - Get sprite velocity
 *
 * Parameters:
 *   sprite — Sprite handle
 *   vx, vy — Pointers to receive velocity
 */
void sprite_get_velocity(sprite_t sprite, float *vx, float *vy);

/**
 * sprite_set_speed_heading - Set sprite velocity (polar)
 *
 * Sets movement using speed and heading angle.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   speed — Speed in pixels per interval
 *   heading — Direction in degrees (0-360)
 *             0° = right, 90° = down, 180° = left, 270° = up
 */
void sprite_set_speed_heading(sprite_t sprite, float speed, float heading);

/**
 * sprite_get_speed_heading - Get sprite speed and heading
 *
 * Parameters:
 *   sprite — Sprite handle
 *   speed, heading — Pointers to receive values
 */
void sprite_get_speed_heading(sprite_t sprite, float *speed, float *heading);

/**
 * sprite_set_acceleration - Set acceleration
 *
 * Parameters:
 *   sprite — Sprite handle
 *   ax, ay — Acceleration components (pixels per interval²)
 */
void sprite_set_acceleration(sprite_t sprite, float ax, float ay);

/**
 * sprite_get_acceleration - Get acceleration
 *
 * Parameters:
 *   sprite — Sprite handle
 *   ax, ay — Pointers to receive acceleration
 */
void sprite_get_acceleration(sprite_t sprite, float *ax, float *ay);

/**
 * sprite_set_friction - Set velocity friction/damping
 *
 * Friction is multiplied against velocity each interval.
 * Useful for deceleration without explicit acceleration.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   friction — Damping factor (0.0 = stop immediately, 1.0 = no friction)
 *              Typical values: 0.95-0.99
 */
void sprite_set_friction(sprite_t sprite, float friction);

/**
 * sprite_get_friction - Get velocity friction
 *
 * Returns:
 *   Current friction factor
 */
float sprite_get_friction(sprite_t sprite);

/**
 * sprite_accelerate - Apply impulse to sprite
 *
 * Adds acceleration over next interval only.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   ax, ay — Impulse components
 */
void sprite_accelerate(sprite_t sprite, float ax, float ay);

/**
 * sprite_set_movement_active - Enable/disable movement
 *
 * Parameters:
 *   sprite — Sprite handle
 *   active — 1 to enable, 0 to disable
 */
void sprite_set_movement_active(sprite_t sprite, int active);

/**
 * sprite_is_movement_active - Check if movement is enabled
 *
 * Returns:
 *   1 if movement enabled, 0 otherwise
 */
int sprite_is_movement_active(sprite_t sprite);

/**
 * sprite_get_movement - Get complete movement state
 *
 * Parameters:
 *   sprite — Sprite handle
 *   movement — Pointer to sprite_movement_t to receive state
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_get_movement(sprite_t sprite, sprite_movement_t *movement);

/* ============================================================================
 * ANIMATION CONTROL
 * ========================================================================== */

/**
 * sprite_set_animation_frames - Set animation frame sequence
 *
 * Associates sprite with frame sequence. Frames are uint8_t bitmaps.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   frames — Array of frame bitmaps (uint8_t*)
 *   frame_count — Number of frames
 *   frame_delay — Intervals per frame (typically 2-10)
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_set_animation_frames(sprite_t sprite, uint8_t **frames,
                                int frame_count, int frame_delay);

/**
 * sprite_play_animation - Start animation playback
 *
 * Parameters:
 *   sprite — Sprite handle
 *   mode — Play mode (ANIM_ONCE, ANIM_LOOP, ANIM_PINGPONG)
 *
 * Returns:
 *   1 if animation started, 0 if no frames assigned
 */
int sprite_play_animation(sprite_t sprite, sprite_anim_mode_t mode);

/**
 * sprite_pause_animation - Pause animation playback
 *
 * Keeps current frame displayed, resumes from same frame.
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_pause_animation(sprite_t sprite);

/**
 * sprite_stop_animation - Stop animation and reset to frame 0
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_stop_animation(sprite_t sprite);

/**
 * sprite_is_animation_playing - Check if animation is running
 *
 * Returns:
 *   1 if playing, 0 if paused or stopped
 */
int sprite_is_animation_playing(sprite_t sprite);

/**
 * sprite_get_animation_frame - Get current animation frame
 *
 * Returns:
 *   Frame index (0 to frame_count-1), or -1 if no animation
 */
int sprite_get_animation_frame(sprite_t sprite);

/**
 * sprite_set_animation_frame - Jump to specific frame
 *
 * Parameters:
 *   sprite — Sprite handle
 *   frame — Frame index
 *
 * Returns:
 *   1 if successful, 0 if invalid frame
 */
int sprite_set_animation_frame(sprite_t sprite, int frame);

/**
 * sprite_get_animation - Get complete animation state
 *
 * Parameters:
 *   sprite — Sprite handle
 *   animation — Pointer to sprite_animation_t to receive state
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_get_animation(sprite_t sprite, sprite_animation_t *animation);

/**
 * sprite_set_on_frame_callback - Register animation frame change callback
 *
 * Called whenever current frame advances.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   callback — Function to call, or NULL to disable
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_set_on_frame_callback(sprite_t sprite,
                                 sprite_on_frame_callback callback);

/**
 * sprite_set_on_move_callback - Register sprite movement callback
 *
 * Called whenever sprite position changes due to velocity.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   callback — Function to call, or NULL to disable
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_set_on_move_callback(sprite_t sprite,
                                sprite_on_move_callback callback);

/* ============================================================================
 * UPDATE SYSTEM
 * ========================================================================== */

/**
 * sprite_update_interval - Update all active sprites
 *
 * Advances movement and animation for all sprites.
 * Call once per frame (or at fixed time step).
 *
 * Parameters:
 *   delta_ms — Elapsed time in milliseconds since last update
 *              Typical: 16 (60fps), 20 (50fps), 33 (30fps)
 *
 * Returns:
 *   Number of sprites updated
 */
int sprite_update_interval(int delta_ms);

/**
 * sprite_update_single - Update single sprite
 *
 * Manually update a specific sprite's movement and animation.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   delta_ms — Elapsed time in milliseconds
 *
 * Returns:
 *   1 if updated, 0 if sprite invalid or not active
 */
int sprite_update_single(sprite_t sprite, int delta_ms);

/**
 * sprite_reset_update_timers - Reset all sprite timers
 *
 * Clears accumulated time for all sprites.
 * Call when pausing/resuming simulation.
 */
void sprite_reset_update_timers(void);

/* ============================================================================
 * PHYSICS UTILITIES
 * ========================================================================== */

/**
 * sprite_apply_gravity - Apply downward acceleration (gravity)
 *
 * Parameters:
 *   sprite — Sprite handle
 *   gravity — Gravity acceleration (pixels per interval²)
 *             Typical: 0.3-0.5
 */
void sprite_apply_gravity(sprite_t sprite, float gravity);

/**
 * sprite_clamp_velocity - Limit maximum velocity
 *
 * Prevents velocity from exceeding a threshold.
 * Useful for preventing sprites from moving too fast.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   max_speed — Maximum speed (pixels per interval)
 */
void sprite_clamp_velocity(sprite_t sprite, float max_speed);

/**
 * sprite_rotate_towards - Rotate sprite towards target
 *
 * Updates heading to point towards (target_x, target_y).
 *
 * Parameters:
 *   sprite — Sprite handle
 *   target_x, target_y — Target coordinates
 *   max_rotation — Maximum rotation per interval (degrees)
 *                  Use 360 for instant rotation
 */
void sprite_rotate_towards(sprite_t sprite, int target_x, int target_y,
                           float max_rotation);

/**
 * sprite_move_towards - Accelerate towards target
 *
 * Sets velocity towards (target_x, target_y) with specified acceleration.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   target_x, target_y — Target coordinates
 *   acceleration — Pixels per interval² towards target
 */
void sprite_move_towards(sprite_t sprite, int target_x, int target_y,
                         float acceleration);

/**
 * sprite_distance_to - Calculate distance to point
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Target coordinates
 *
 * Returns:
 *   Distance in pixels (floating point)
 */
float sprite_distance_to(sprite_t sprite, int x, int y);

/**
 * sprite_distance_to_sprite - Calculate distance between sprites
 *
 * Parameters:
 *   sprite1, sprite2 — Sprite handles
 *
 * Returns:
 *   Distance in pixels (floating point)
 */
float sprite_distance_to_sprite(sprite_t sprite1, sprite_t sprite2);

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * sprite_print_dynamics - Print movement and animation state
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_print_dynamics(sprite_t sprite);

#endif
