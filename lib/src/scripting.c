/* sprite_scripting.c — Scripting Framework Implementation */

#include "scripting.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_SCRIPTS 32
#define MAX_STATE_MACHINES 16
#define MAX_STATES 64
#define MAX_TRANSITIONS 128

typedef struct {
    void *context;
    sprite_script_update_t update_fn;
} script_impl;

typedef struct {
    char name[64];
    sprite_script_callback_t on_enter;
    sprite_script_callback_t on_exit;
    sprite_script_update_t on_update;
} state_impl;

typedef struct {
    char from_state[64];
    char to_state[64];
    sprite_script_callback_t condition;
} transition_impl;

typedef struct {
    state_impl states[MAX_STATES];
    int state_count;
    transition_impl transitions[MAX_TRANSITIONS];
    int transition_count;
    int current_state_idx;
    char current_state[64];
} state_machine_impl;

static script_impl scripts[MAX_SCRIPTS];
static int script_count = 0;

static state_machine_impl state_machines[MAX_STATE_MACHINES];
static int state_machine_count = 0;

sprite_script_t sprite_script_create(void *context) {
    if (script_count >= MAX_SCRIPTS) return INVALID_SCRIPT;

    script_impl *script = &scripts[script_count];
    script->context = context;
    script->update_fn = NULL;

    return (sprite_script_t)(intptr_t)script_count++;
}

void sprite_script_destroy(sprite_script_t script) {}

int sprite_script_update(sprite_script_t script, int delta_ms) {
    intptr_t idx = (intptr_t)script;
    if (idx < 0 || idx >= script_count) return 0;

    script_impl *s = &scripts[idx];
    if (s->update_fn) {
        return s->update_fn(s->context, delta_ms);
    }
    return 1;
}

void sprite_script_set_context(sprite_script_t script, void *context) {
    intptr_t idx = (intptr_t)script;
    if (idx < 0 || idx >= script_count) return;
    scripts[idx].context = context;
}

sprite_state_machine_t sprite_state_machine_create(const char *initial_state) {
    if (state_machine_count >= MAX_STATE_MACHINES) return INVALID_STATE_MACHINE;

    state_machine_impl *machine = &state_machines[state_machine_count];
    machine->state_count = 0;
    machine->transition_count = 0;
    machine->current_state_idx = 0;
    strncpy(machine->current_state, initial_state, sizeof(machine->current_state) - 1);

    return (sprite_state_machine_t)(intptr_t)state_machine_count++;
}

void sprite_state_machine_destroy(sprite_state_machine_t machine) {}

int sprite_state_machine_add_state(sprite_state_machine_t machine, sprite_state_def_t state) {
    intptr_t idx = (intptr_t)machine;
    if (idx < 0 || idx >= state_machine_count) return 0;

    state_machine_impl *m = &state_machines[idx];
    if (m->state_count >= MAX_STATES) return 0;

    state_impl *s = &m->states[m->state_count];
    strncpy(s->name, state.name, sizeof(s->name) - 1);
    s->on_enter = state.on_enter;
    s->on_exit = state.on_exit;
    s->on_update = state.on_update;

    return ++m->state_count;
}

int sprite_state_machine_add_transition(sprite_state_machine_t machine, sprite_transition_def_t transition) {
    intptr_t idx = (intptr_t)machine;
    if (idx < 0 || idx >= state_machine_count) return 0;

    state_machine_impl *m = &state_machines[idx];
    if (m->transition_count >= MAX_TRANSITIONS) return 0;

    transition_impl *t = &m->transitions[m->transition_count];
    strncpy(t->from_state, transition.from_state, sizeof(t->from_state) - 1);
    strncpy(t->to_state, transition.to_state, sizeof(t->to_state) - 1);
    t->condition = transition.condition;

    return ++m->transition_count;
}

int sprite_state_machine_update(sprite_state_machine_t machine, void *context, int delta_ms) {
    intptr_t idx = (intptr_t)machine;
    if (idx < 0 || idx >= state_machine_count) return 0;

    state_machine_impl *m = &state_machines[idx];

    for (int i = 0; i < m->transition_count; i++) {
        transition_impl *t = &m->transitions[i];
        if (strcmp(t->from_state, m->current_state) == 0) {
            if (t->condition && t->condition(context)) {
                strncpy(m->current_state, t->to_state, sizeof(m->current_state) - 1);
                break;
            }
        }
    }

    for (int i = 0; i < m->state_count; i++) {
        if (strcmp(m->states[i].name, m->current_state) == 0) {
            if (m->states[i].on_update) {
                m->states[i].on_update(context, delta_ms);
            }
            break;
        }
    }

    return 1;
}

const char *sprite_state_machine_get_current_state(sprite_state_machine_t machine) {
    intptr_t idx = (intptr_t)machine;
    if (idx < 0 || idx >= state_machine_count) return "";
    return state_machines[idx].current_state;
}

int sprite_state_machine_set_state(sprite_state_machine_t machine, const char *state, void *context) {
    intptr_t idx = (intptr_t)machine;
    if (idx < 0 || idx >= state_machine_count) return 0;

    state_machine_impl *m = &state_machines[idx];

    for (int i = 0; i < m->state_count; i++) {
        if (strcmp(m->states[i].name, state) == 0) {
            strncpy(m->current_state, state, sizeof(m->current_state) - 1);
            if (m->states[i].on_enter) {
                m->states[i].on_enter(context);
            }
            return 1;
        }
    }

    return 0;
}

sprite_behavior_t sprite_behavior_create(sprite_behavior_node_t root) {
    if (!root) return INVALID_BEHAVIOR;
    return (sprite_behavior_t)root;
}

void sprite_behavior_destroy(sprite_behavior_t behavior) {}

int sprite_behavior_update(sprite_behavior_t behavior, void *context) {
    sprite_behavior_node_t node = (sprite_behavior_node_t)behavior;
    if (node) {
        return node(context);
    }
    return 0;
}

int sprite_condition_true(void *context) {
    return 1;
}

int sprite_condition_false(void *context) {
    return 0;
}

int sprite_condition_timer_elapsed(void *context) {
    return 1;
}

int sprite_action_noop(void *context) {
    return 1;
}

int sprite_action_print(void *context) {
    return 1;
}
