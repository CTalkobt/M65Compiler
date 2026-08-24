/* graphics_sprites_advanced.h — Advanced Sprite Features (Phase 8)
 *
 * Animation sequences, movement patterns, sprite groups, and callbacks.
 */

#pragma once

#include <graphics_sprites.h>

/* ============================================================================
 * ADVANCED SPRITE FEATURES (Phase 8)
 * ============================================================================ */

/**
 * Sprite event types
 */
typedef enum {
    SPRITE_EVENT_ANIMATION_END,     /* Animation finished playing */
    SPRITE_EVENT_COLLISION,         /* Collision detected */
    SPRITE_EVENT_OUT_OF_BOUNDS,     /* Moved outside screen */
    SPRITE_EVENT_CUSTOM             /* Application-specific event */
} sprite_event_type_t;

/**
 * Event callback
 */
typedef void (*sprite_event_callback_t)(sprite_t *sprite, sprite_event_type_t evt, int param);

/**
 * Movement pattern type
 */
typedef enum {
    PATTERN_LINEAR,                 /* Straight line movement */
    PATTERN_SINE,                   /* Sinusoidal movement */
    PATTERN_CIRCLE,                 /* Circular/orbital movement */
    PATTERN_ZIGZAG,                 /* Zigzag pattern */
    PATTERN_BOUNCE                  /* Bouncing movement */
} movement_pattern_type_t;

/**
 * Movement pattern definition
 */
typedef struct {
    movement_pattern_type_t type;   /* Pattern type */
    int amplitude;                  /* Movement amplitude (pixels) */
    int frequency;                  /* Pattern frequency */
    int duration;                   /* Total duration (frames, 0=infinite) */
    int elapsed;                    /* Elapsed time (internal) */
    int base_x, base_y;            /* Base position for oscillation */
} sprite_pattern_t;

/**
 * Animation sequence (multiple animations chained together)
 */
typedef struct {
    sprite_animation_t *animations;  /* Array of animations */
    int count;                       /* Number of animations in sequence */
    int current_animation;           /* Current animation index */
    int auto_advance;               /* 1 = auto-advance to next, 0 = manual */
} sprite_animation_sequence_t;

/**
 * Sprite group for batch operations
 */
typedef struct {
    sprite_t **sprites;             /* Array of sprite pointers */
    int capacity;                   /* Maximum sprites in group */
    int count;                      /* Current sprites in group */
} sprite_group_t;

/* ============================================================================
 * ANIMATION SEQUENCE FUNCTIONS
 * ============================================================================ */

/**
 * Create animation sequence from array
 *
 * Parameters:
 *   animations — Array of sprite_animation_t
 *   count — Number of animations
 *   auto_advance — 1 = auto-advance when animation ends, 0 = manual
 *
 * Returns:
 *   Initialized animation sequence
 */
sprite_animation_sequence_t sprite_animation_sequence_create(
    sprite_animation_t *animations, int count, int auto_advance);

/**
 * Update animation sequence
 *
 * Advances frames and handles transitions between animations.
 */
void sprite_animation_sequence_update(sprite_t *sprite, sprite_animation_sequence_t *seq);

/**
 * Advance to next animation in sequence
 */
void sprite_animation_sequence_next(sprite_t *sprite, sprite_animation_sequence_t *seq);

/**
 * Get current animation index
 */
int sprite_animation_sequence_current(sprite_animation_sequence_t *seq);

/**
 * Cleanup animation sequence
 */
void sprite_animation_sequence_done(sprite_animation_sequence_t *seq);

/* ============================================================================
 * MOVEMENT PATTERN FUNCTIONS
 * ============================================================================ */

/**
 * Create movement pattern
 *
 * Parameters:
 *   type — Pattern type (LINEAR, SINE, CIRCLE, etc.)
 *   amplitude — Movement amplitude in pixels
 *   frequency — Pattern frequency/speed
 *   duration — Duration in frames (0 = infinite)
 *
 * Returns:
 *   Initialized movement pattern
 */
sprite_pattern_t sprite_pattern_create(movement_pattern_type_t type, int amplitude,
                                       int frequency, int duration);

/**
 * Apply movement pattern to sprite
 *
 * Updates sprite position based on pattern type.
 */
void sprite_pattern_apply(sprite_t *sprite, sprite_pattern_t *pattern);

/**
 * Check if movement pattern is finished
 *
 * Returns:
 *   1 if pattern duration elapsed, 0 otherwise
 */
int sprite_pattern_finished(sprite_pattern_t *pattern);

/**
 * Reset movement pattern
 */
void sprite_pattern_reset(sprite_pattern_t *pattern);

/* ============================================================================
 * SPRITE GROUP FUNCTIONS
 * ============================================================================ */

/**
 * Create sprite group
 *
 * Parameters:
 *   capacity — Maximum sprites in group
 *
 * Returns:
 *   Initialized sprite group
 */
sprite_group_t sprite_group_create(int capacity);

/**
 * Add sprite to group
 *
 * Returns:
 *   1 if added successfully, 0 if group full
 */
int sprite_group_add(sprite_group_t *group, sprite_t *sprite);

/**
 * Remove sprite from group
 *
 * Returns:
 *   1 if removed, 0 if not found
 */
int sprite_group_remove(sprite_group_t *group, sprite_t *sprite);

/**
 * Get sprite count in group
 */
int sprite_group_count(sprite_group_t *group);

/**
 * Clear all sprites from group (does not free sprites)
 */
void sprite_group_clear(sprite_group_t *group);

/**
 * Update all sprites in group
 */
void sprite_group_update_all(sprite_group_t *group);

/**
 * Render all sprites in group to RRB layer
 */
void sprite_group_render_all(sprite_group_t *group, rrb_layer_t *layer);

/**
 * Check collisions between all sprites in group
 *
 * Callback is called for each collision pair found.
 */
void sprite_group_check_collisions(sprite_group_t *group,
                                   sprite_event_callback_t evt_callback);

/**
 * Destroy sprite group
 */
void sprite_group_destroy(sprite_group_t *group);

/* ============================================================================
 * EVENT CALLBACK SYSTEM
 * ============================================================================ */

/**
 * Set event callback for sprite
 */
void sprite_set_event_callback(sprite_t *sprite, sprite_event_callback_t evt_callback);

/**
 * Dispatch event to sprite callback
 */
void sprite_dispatch_event(sprite_t *sprite, sprite_event_type_t evt, int param);

/**
 * Check if sprite is out of bounds
 *
 * Parameters:
 *   max_x, max_y — Screen dimensions
 *
 * Returns:
 *   1 if out of bounds, 0 otherwise
 */
int sprite_is_out_of_bounds(sprite_t *sprite, int max_x, int max_y);

/* ============================================================================
 * ADVANCED SPRITE OPERATIONS
 * ============================================================================ */

/**
 * Set sprite scale factor (visual scaling via duplicate characters)
 *
 * Not actual scaling; approximates via character repetition.
 */
void sprite_set_scale(sprite_t *sprite, int scale_percent);

/**
 * Get sprite scale factor
 */
int sprite_get_scale(sprite_t *sprite);

/**
 * Apply force to sprite (adds to velocity)
 *
 * Parameters:
 *   fx, fy — Force components
 */
void sprite_apply_force(sprite_t *sprite, int fx, int fy);

/**
 * Calculate distance between two sprites
 *
 * Returns:
 *   Distance in pixels (approximate)
 */
int sprite_distance(sprite_t *sprite1, sprite_t *sprite2);

/**
 * Move sprite towards target
 *
 * Parameters:
 *   target_x, target_y — Target position
 *   speed — Movement speed (pixels/frame)
 */
void sprite_move_towards(sprite_t *sprite, int target_x, int target_y, int speed);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SPRITE_GROUP_MAX_SPRITES    256     /* Maximum sprites per group */
#define SPRITE_MAX_SCALE            400     /* Maximum scale percentage */
