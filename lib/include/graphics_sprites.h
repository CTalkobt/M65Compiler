/* graphics_sprites.h — Soft Sprite System (Phase 7)
 *
 * Object-oriented sprite engine using RRB SPARSE layers for rendering.
 * Supports animation, physics, collision detection, and object pooling.
 */

#pragma once

#include <graphics_rrb.h>

/* ============================================================================
 * SPRITE DEFINITIONS (Phase 7)
 * ============================================================================ */

/**
 * Sprite animation frame
 */
typedef struct {
    unsigned char ch;           /* Character to display */
    unsigned char color;        /* Color for this frame */
    int duration;               /* Frames to display this frame (0 = infinite) */
} sprite_frame_t;

/**
 * Sprite animation state
 */
typedef struct {
    sprite_frame_t *frames;     /* Array of animation frames */
    int frame_count;            /* Number of frames */
    int current_frame;          /* Current frame index */
    int frame_timer;            /* Frames remaining in current frame */
    int is_looping;             /* 1 = loop animation, 0 = stop at end */
    int is_playing;             /* 1 = animation active, 0 = paused */
} sprite_animation_t;

/**
 * Sprite collision box (for detection)
 */
typedef struct {
    int x_offset;               /* Offset from sprite position */
    int y_offset;
    int width;                  /* Width in pixels */
    int height;                 /* Height in pixels */
    int enabled;                /* 1 = check collisions, 0 = skip */
} sprite_collision_box_t;

/**
 * Soft sprite definition
 *
 * Rendered as a single character on SPARSE RRB layer.
 * Supports position, velocity, animation, and collision detection.
 */
typedef struct sprite {
    /* Position and movement */
    int x;                      /* X position (pixels) */
    int y;                      /* Y position (pixels) */
    int vx;                     /* X velocity (pixels/frame) */
    int vy;                     /* Y velocity (pixels/frame) */
    int ax;                     /* X acceleration (pixels/frame²) */
    int ay;                     /* Y acceleration (pixels/frame²) */

    /* Rendering */
    unsigned char ch;           /* Current character (may be overridden by animation) */
    unsigned char color;        /* Current color */
    int visible;                /* 1 = visible, 0 = hidden */
    int layer_index;            /* RRB layer this sprite renders to */

    /* Animation */
    sprite_animation_t animation;

    /* Collision */
    sprite_collision_box_t collision_box;
    int collision_group;        /* Group ID for filtering collisions */

    /* State */
    int active;                 /* 1 = active, 0 = inactive (pooled) */
    int user_data;              /* Application-specific data */

    /* Methods */

    void (*set_position)(struct sprite *this, int x, int y);
    void (*set_velocity)(struct sprite *this, int vx, int vy);
    void (*set_acceleration)(struct sprite *this, int ax, int ay);
    void (*set_animation)(struct sprite *this, sprite_animation_t *anim);
    void (*play_animation)(struct sprite *this);
    void (*stop_animation)(struct sprite *this);
    void (*show)(struct sprite *this);
    void (*hide)(struct sprite *this);
    void (*update)(struct sprite *this);
    void (*render)(struct sprite *this, rrb_layer_t *layer);

} sprite_t;

/**
 * Sprite manager (object pool)
 *
 * Manages sprite allocation, update, and rendering.
 */
typedef struct {
    sprite_t *sprites;          /* Array of sprites */
    int max_sprites;            /* Maximum sprites in pool */
    int active_count;           /* Currently active sprites */

    rrb_system_t *rrb;          /* Associated RRB system */
    int sprite_layer_index;     /* RRB layer for sprite rendering */

    /* Methods */

    sprite_t *(*allocate)(struct sprite_manager *this);
    void (*free)(struct sprite_manager *this, sprite_t *sprite);
    void (*update_all)(struct sprite_manager *this);
    void (*render_all)(struct sprite_manager *this);
    int (*get_active_count)(struct sprite_manager *this);
    void (*clear)(struct sprite_manager *this);

} sprite_manager_t;

/* ============================================================================
 * SPRITE API FUNCTIONS
 * ============================================================================ */

/**
 * Create and initialize a sprite manager
 *
 * Parameters:
 *   max_sprites — Maximum sprites in pool
 *   rrb — Associated RRB system
 *   layer_index — RRB SPARSE layer for rendering
 *
 * Returns:
 *   Initialized sprite manager
 */
sprite_manager_t sprite_manager_create(int max_sprites, rrb_system_t *rrb, int layer_index);

/**
 * Destroy sprite manager and free resources
 */
void sprite_manager_destroy(sprite_manager_t *manager);

/**
 * Allocate sprite from pool
 *
 * Returns:
 *   Allocated sprite, or NULL if pool exhausted
 */
sprite_t *sprite_allocate(sprite_manager_t *manager);

/**
 * Free sprite back to pool
 */
void sprite_free(sprite_manager_t *manager, sprite_t *sprite);

/**
 * Update all active sprites (physics, animation)
 */
void sprite_update_all(sprite_manager_t *manager);

/**
 * Render all active sprites to RRB layer
 */
void sprite_render_all(sprite_manager_t *manager);

/**
 * Check collision between two sprites
 *
 * Returns:
 *   1 if collided, 0 if not
 */
int sprite_collides(sprite_t *sprite1, sprite_t *sprite2);

/**
 * Create animation from frame array
 *
 * Parameters:
 *   frames — Array of sprite_frame_t
 *   count — Number of frames
 *   looping — 1 = loop animation, 0 = play once
 *
 * Returns:
 *   Initialized sprite_animation_t
 */
sprite_animation_t sprite_animation_create(sprite_frame_t *frames, int count, int looping);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define SPRITE_MAX_SPRITES      256     /* Maximum sprites in pool */
#define SPRITE_ANIMATION_FRAMES 64      /* Max frames per animation */

/* Sprite state flags */
#define SPRITE_VISIBLE          1
#define SPRITE_HIDDEN           0
#define SPRITE_ACTIVE           1
#define SPRITE_INACTIVE         0
