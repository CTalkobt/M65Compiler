/* sprite_animation_trees.h — Hierarchical Animation System for MEGA65
 *
 * Provides time-based animation playback across sprite hierarchies.
 * Enables synchronized animations of parent and children with state machines.
 *
 * Features:
 * - Animation layers (base, override, blend)
 * - State machines with transitions
 * - Synchronized parent-child animation
 * - Animation blending and crossfade
 * - Time scaling (slow-motion, fast-forward)
 * - Event callbacks at animation milestones
 *
 * Usage:
 *   #include <sprite_animation_trees.h>
 *   sprite_anim_tree_t tree = sprite_anim_tree_create(root_sprite);
 *   sprite_anim_layer_t base = sprite_anim_layer_add(tree, "base", 1.0f);
 *   sprite_anim_state_t idle = sprite_anim_state_add(base, "idle");
 *   sprite_anim_state_add_clip(idle, idle_frames, 4, 2);
 *   sprite_anim_tree_play_state(tree, "idle");
 *   sprite_anim_tree_update(tree, 16);  // 60fps update
 */

#ifndef SPRITE_ANIMATION_TREES_H
#define SPRITE_ANIMATION_TREES_H

#include <sprites.h>
#include <sprite_groups.h>
#include <stdint.h>

/* ============================================================================
 * ANIMATION TREE TYPES
 * ========================================================================== */

typedef void *sprite_anim_tree_t;       /* Animation tree handle */
typedef void *sprite_anim_layer_t;      /* Animation layer handle */
typedef void *sprite_anim_state_t;      /* Animation state handle */
typedef void *sprite_anim_clip_t;       /* Animation clip handle */
typedef void *sprite_anim_transition_t; /* State transition handle */

#define INVALID_ANIM_TREE       NULL
#define INVALID_ANIM_LAYER      NULL
#define INVALID_ANIM_STATE      NULL
#define INVALID_ANIM_CLIP       NULL
#define INVALID_ANIM_TRANSITION NULL

/* ============================================================================
 * ANIMATION STRUCTURES
 * ========================================================================== */

typedef enum {
    ANIM_LAYER_BASE = 0,      /* Base animation layer */
    ANIM_LAYER_OVERRIDE = 1,  /* Override layer (highest priority) */
    ANIM_LAYER_ADDITIVE = 2,  /* Additive blending layer */
    ANIM_LAYER_BLEND = 3,     /* Blend layer with alpha */
} sprite_anim_layer_type_t;

typedef enum {
    ANIM_TRANSITION_IMMEDIATE = 0,  /* Switch instantly */
    ANIM_TRANSITION_CROSSFADE = 1,  /* Blend between states */
    ANIM_TRANSITION_QUEUE = 2,      /* Queue after current finishes */
} sprite_anim_transition_type_t;

typedef struct {
    sprite_anim_tree_t tree;    /* Parent tree */
    int state_count;            /* Number of states */
    int active_state_index;     /* Current active state */
    float playback_time;        /* Current playback time */
    float time_scale;           /* Playback speed (1.0 = normal) */
    int is_playing;             /* Playback active flag */
} sprite_anim_tree_info_t;

/* ============================================================================
 * TREE CREATION & CONTROL
 * ========================================================================== */

/**
 * sprite_anim_tree_create - Create animation tree for sprite hierarchy
 *
 * Creates a new animation tree bound to a sprite or sprite group.
 * The tree manages synchronized animations across parent and children.
 *
 * Parameters:
 *   root — Root sprite or group for animation tree
 *   is_group — 1 if root is sprite_group_t, 0 if sprite_t
 *
 * Returns:
 *   Animation tree handle on success, INVALID_ANIM_TREE on error
 */
sprite_anim_tree_t sprite_anim_tree_create(void *root, int is_group);

/**
 * sprite_anim_tree_destroy - Destroy animation tree
 *
 * Parameters:
 *   tree — Tree handle to destroy
 */
void sprite_anim_tree_destroy(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_get_info - Get tree information
 *
 * Parameters:
 *   tree — Tree handle
 *   info — Pointer to sprite_anim_tree_info_t to receive data
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_anim_tree_get_info(sprite_anim_tree_t tree,
                               sprite_anim_tree_info_t *info);

/**
 * sprite_anim_tree_update - Update tree animations
 *
 * Advances all active animations by delta time.
 * Should be called once per frame.
 *
 * Parameters:
 *   tree — Tree handle
 *   delta_ms — Elapsed time in milliseconds
 *
 * Returns:
 *   Number of sprites updated
 */
int sprite_anim_tree_update(sprite_anim_tree_t tree, int delta_ms);

/**
 * sprite_anim_tree_set_time_scale - Set playback speed
 *
 * Scales animation playback speed globally for entire tree.
 *
 * Parameters:
 *   tree — Tree handle
 *   scale — Time scale (0.5 = half speed, 2.0 = double speed)
 */
void sprite_anim_tree_set_time_scale(sprite_anim_tree_t tree, float scale);

/**
 * sprite_anim_tree_get_time_scale - Get playback speed
 *
 * Returns:
 *   Current time scale value
 */
float sprite_anim_tree_get_time_scale(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_play - Start playback
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_play(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_pause - Pause playback
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_pause(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_stop - Stop and reset
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_stop(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_is_playing - Check playback status
 *
 * Returns:
 *   1 if playing, 0 if paused or stopped
 */
int sprite_anim_tree_is_playing(sprite_anim_tree_t tree);

/* ============================================================================
 * LAYER MANAGEMENT
 * ========================================================================== */

/**
 * sprite_anim_layer_add - Add animation layer to tree
 *
 * Layers allow multiple animations to blend together.
 * Layers are evaluated in order (base → override).
 *
 * Parameters:
 *   tree — Tree handle
 *   name — Layer name (for state playback)
 *   weight — Blend weight (0.0-1.0, 1.0 = fully visible)
 *   type — Layer type (base, override, additive, blend)
 *
 * Returns:
 *   Layer handle on success, INVALID_ANIM_LAYER on error
 */
sprite_anim_layer_t sprite_anim_layer_add(sprite_anim_tree_t tree,
                                           const char *name, float weight,
                                           sprite_anim_layer_type_t type);

/**
 * sprite_anim_layer_get_weight - Get layer blend weight
 *
 * Returns:
 *   Layer weight (0.0-1.0)
 */
float sprite_anim_layer_get_weight(sprite_anim_layer_t layer);

/**
 * sprite_anim_layer_set_weight - Set layer blend weight
 *
 * Parameters:
 *   layer — Layer handle
 *   weight — Blend weight (0.0-1.0)
 */
void sprite_anim_layer_set_weight(sprite_anim_layer_t layer, float weight);

/**
 * sprite_anim_layer_get_count - Get number of layers
 *
 * Parameters:
 *   tree — Tree handle
 *
 * Returns:
 *   Number of layers in tree
 */
int sprite_anim_layer_get_count(sprite_anim_tree_t tree);

/**
 * sprite_anim_layer_get - Get layer by index
 *
 * Parameters:
 *   tree — Tree handle
 *   index — Layer index (0 to count-1)
 *
 * Returns:
 *   Layer handle, or INVALID_ANIM_LAYER if index out of range
 */
sprite_anim_layer_t sprite_anim_layer_get(sprite_anim_tree_t tree, int index);

/* ============================================================================
 * STATE MANAGEMENT
 * ========================================================================== */

/**
 * sprite_anim_state_add - Add animation state to layer
 *
 * States are named animation configurations that can be played.
 *
 * Parameters:
 *   layer — Layer handle
 *   name — State name (e.g., "idle", "run", "jump")
 *
 * Returns:
 *   State handle on success, INVALID_ANIM_STATE on error
 */
sprite_anim_state_t sprite_anim_state_add(sprite_anim_layer_t layer,
                                           const char *name);

/**
 * sprite_anim_state_add_clip - Add animation clip to state
 *
 * Clips are frame sequences that play in sequence within a state.
 *
 * Parameters:
 *   state — State handle
 *   frames — Array of frame bitmaps
 *   frame_count — Number of frames
 *   frame_delay — Intervals per frame
 *
 * Returns:
 *   Clip handle on success, INVALID_ANIM_CLIP on error
 */
sprite_anim_clip_t sprite_anim_state_add_clip(sprite_anim_state_t state,
                                               uint8_t **frames,
                                               int frame_count,
                                               int frame_delay);

/**
 * sprite_anim_state_get_count - Get number of states in layer
 *
 * Parameters:
 *   layer — Layer handle
 *
 * Returns:
 *   Number of states
 */
int sprite_anim_state_get_count(sprite_anim_layer_t layer);

/**
 * sprite_anim_state_get - Get state by index
 *
 * Parameters:
 *   layer — Layer handle
 *   index — State index (0 to count-1)
 *
 * Returns:
 *   State handle, or INVALID_ANIM_STATE if index out of range
 */
sprite_anim_state_t sprite_anim_state_get(sprite_anim_layer_t layer, int index);

/**
 * sprite_anim_state_find - Find state by name
 *
 * Parameters:
 *   layer — Layer handle
 *   name — State name to find
 *
 * Returns:
 *   State handle, or INVALID_ANIM_STATE if not found
 */
sprite_anim_state_t sprite_anim_state_find(sprite_anim_layer_t layer,
                                            const char *name);

/* ============================================================================
 * STATE PLAYBACK
 * ========================================================================== */

/**
 * sprite_anim_tree_play_state - Play animation state
 *
 * Starts playback of named state from all layers.
 *
 * Parameters:
 *   tree — Tree handle
 *   state_name — Name of state to play
 *   transition_type — How to transition (immediate/crossfade/queue)
 *
 * Returns:
 *   1 if successful, 0 if state not found
 */
int sprite_anim_tree_play_state(sprite_anim_tree_t tree,
                                 const char *state_name,
                                 sprite_anim_transition_type_t transition_type);

/**
 * sprite_anim_tree_queue_state - Queue state for playback
 *
 * Adds state to playback queue (plays after current state finishes).
 *
 * Parameters:
 *   tree — Tree handle
 *   state_name — Name of state to queue
 *
 * Returns:
 *   1 if successful, 0 if state not found
 */
int sprite_anim_tree_queue_state(sprite_anim_tree_t tree,
                                  const char *state_name);

/**
 * sprite_anim_tree_get_current_state - Get currently playing state
 *
 * Returns:
 *   State name (allocated string, caller must free), or NULL if no state
 */
char *sprite_anim_tree_get_current_state(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_is_state_finished - Check if state playback complete
 *
 * Returns:
 *   1 if current state finished, 0 otherwise
 */
int sprite_anim_tree_is_state_finished(sprite_anim_tree_t tree);

/* ============================================================================
 * TRANSITIONS
 * ========================================================================== */

/**
 * sprite_anim_transition_add - Add state transition rule
 *
 * Defines automatic transitions between states based on conditions.
 *
 * Parameters:
 *   from_state — Source state
 *   to_state — Destination state
 *   condition — Condition name (user-defined)
 *   transition_type — How to transition
 *
 * Returns:
 *   Transition handle on success, INVALID_ANIM_TRANSITION on error
 */
sprite_anim_transition_t sprite_anim_transition_add(sprite_anim_state_t from_state,
                                                     sprite_anim_state_t to_state,
                                                     const char *condition,
                                                     sprite_anim_transition_type_t type);

/**
 * sprite_anim_tree_trigger_transition - Trigger named transition
 *
 * Parameters:
 *   tree — Tree handle
 *   condition — Condition name to trigger
 *
 * Returns:
 *   1 if transition triggered, 0 if condition not found
 */
int sprite_anim_tree_trigger_transition(sprite_anim_tree_t tree,
                                         const char *condition);

/**
 * sprite_anim_transition_remove - Remove transition rule
 *
 * Parameters:
 *   transition — Transition handle to remove
 */
void sprite_anim_transition_remove(sprite_anim_transition_t transition);

/* ============================================================================
 * CALLBACKS & EVENTS
 * ========================================================================== */

typedef void (*sprite_anim_callback)(sprite_anim_tree_t tree, const char *event);

/**
 * sprite_anim_tree_set_callback - Register animation event callback
 *
 * Callback called on animation events:
 * - "state_start" — State playback started
 * - "state_end" — State playback finished
 * - "clip_change" — Frame clip changed
 *
 * Parameters:
 *   tree — Tree handle
 *   callback — Function to call, or NULL to disable
 */
void sprite_anim_tree_set_callback(sprite_anim_tree_t tree,
                                    sprite_anim_callback callback);

/* ============================================================================
 * SYNCHRONIZATION
 * ========================================================================== */

/**
 * sprite_anim_tree_synchronize_children - Sync child animations with parent
 *
 * Forces all child animations to match parent state and timing.
 * Used after modifying parent animation.
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_synchronize_children(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_get_child_trees - Get animation trees for children
 *
 * Retrieves list of child animation trees in hierarchy.
 *
 * Parameters:
 *   tree — Parent tree handle
 *   children — Array to receive child tree handles (pre-allocated)
 *   max_children — Size of children array
 *
 * Returns:
 *   Number of child trees found (may be > max_children)
 */
int sprite_anim_tree_get_child_trees(sprite_anim_tree_t tree,
                                      sprite_anim_tree_t *children,
                                      int max_children);

/* ============================================================================
 * QUERYING & INSPECTION
 * ========================================================================== */

/**
 * sprite_anim_tree_get_playback_time - Get current playback position
 *
 * Returns:
 *   Current time in milliseconds
 */
int sprite_anim_tree_get_playback_time(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_set_playback_time - Seek to time
 *
 * Parameters:
 *   tree — Tree handle
 *   time_ms — Time to seek to (milliseconds)
 */
void sprite_anim_tree_set_playback_time(sprite_anim_tree_t tree, int time_ms);

/**
 * sprite_anim_state_get_duration - Get state playback duration
 *
 * Returns duration in milliseconds for complete state playback.
 *
 * Parameters:
 *   state — State handle
 *
 * Returns:
 *   Duration in milliseconds, or 0 if state has no clips
 */
int sprite_anim_state_get_duration(sprite_anim_state_t state);

/**
 * sprite_anim_tree_print_state - Print tree state (debug)
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_print_state(sprite_anim_tree_t tree);

/**
 * sprite_anim_tree_print_hierarchy - Print tree hierarchy (debug)
 *
 * Parameters:
 *   tree — Tree handle
 */
void sprite_anim_tree_print_hierarchy(sprite_anim_tree_t tree);

#endif
