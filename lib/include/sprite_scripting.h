/* sprite_scripting.h — Phase 65: Scripting/Gameplay Logic Framework
 *
 * Lightweight scripting and state machine system for gameplay logic.
 */

#ifndef SPRITE_SCRIPTING_H
#define SPRITE_SCRIPTING_H

typedef void *sprite_script_t;
typedef void *sprite_state_machine_t;
typedef void *sprite_behavior_t;

#define INVALID_SCRIPT NULL
#define INVALID_STATE_MACHINE NULL
#define INVALID_BEHAVIOR NULL

typedef int (*sprite_script_update_t)(void *context, int delta_ms);
typedef int (*sprite_script_callback_t)(void *context);

typedef struct {
    const char *name;
    sprite_script_callback_t on_enter;
    sprite_script_callback_t on_exit;
    sprite_script_update_t on_update;
} sprite_state_def_t;

typedef struct {
    const char *from_state;
    const char *to_state;
    sprite_script_callback_t condition;
} sprite_transition_def_t;

/* Script System */
sprite_script_t sprite_script_create(void *context);
void sprite_script_destroy(sprite_script_t script);
int sprite_script_update(sprite_script_t script, int delta_ms);
void sprite_script_set_context(sprite_script_t script, void *context);

/* State Machine */
sprite_state_machine_t sprite_state_machine_create(const char *initial_state);
void sprite_state_machine_destroy(sprite_state_machine_t machine);
int sprite_state_machine_add_state(sprite_state_machine_t machine, sprite_state_def_t state);
int sprite_state_machine_add_transition(sprite_state_machine_t machine, sprite_transition_def_t transition);
int sprite_state_machine_update(sprite_state_machine_t machine, void *context, int delta_ms);
const char *sprite_state_machine_get_current_state(sprite_state_machine_t machine);
int sprite_state_machine_set_state(sprite_state_machine_t machine, const char *state, void *context);

/* Behavior Tree */
typedef int (*sprite_behavior_node_t)(void *context);

sprite_behavior_t sprite_behavior_create(sprite_behavior_node_t root);
void sprite_behavior_destroy(sprite_behavior_t behavior);
int sprite_behavior_update(sprite_behavior_t behavior, void *context);

/* Condition Helpers */
int sprite_condition_true(void *context);
int sprite_condition_false(void *context);
int sprite_condition_timer_elapsed(void *context);

/* Action Helpers */
int sprite_action_noop(void *context);
int sprite_action_print(void *context);

#endif
