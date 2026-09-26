/* sprite_animation_trees.c — Hierarchical Animation System Implementation
 *
 * Provides time-based animation playback across sprite hierarchies.
 */

#include <sprite_animation_trees.h>
#include <sprite_dynamics.h>
#include <sprite_groups.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_TREES 64
#define MAX_LAYERS_PER_TREE 16
#define MAX_STATES_PER_LAYER 32
#define MAX_CLIPS_PER_STATE 16
#define MAX_TRANSITIONS_PER_STATE 8
#define MAX_NAME_LEN 64

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ========================================================================== */

typedef struct {
    char name[MAX_NAME_LEN];
    uint8_t **frames;
    int frame_count;
    int frame_delay;
    int total_duration;
} anim_clip_t;

typedef struct {
    char name[MAX_NAME_LEN];
    anim_clip_t clips[MAX_CLIPS_PER_STATE];
    int clip_count;
} anim_state_t;

typedef struct {
    char name[MAX_NAME_LEN];
    anim_state_t states[MAX_STATES_PER_LAYER];
    int state_count;
    float weight;
    sprite_anim_layer_type_t type;
} anim_layer_t;

typedef struct {
    char from_state[MAX_NAME_LEN];
    char to_state[MAX_NAME_LEN];
    char condition[MAX_NAME_LEN];
    sprite_anim_transition_type_t type;
    int active;
} anim_transition_t;

typedef struct {
    void *root;
    int is_group;
    anim_layer_t layers[MAX_LAYERS_PER_TREE];
    int layer_count;
    int active_state_index;
    float playback_time;
    float time_scale;
    int is_playing;
    sprite_anim_callback callback;
    anim_transition_t transitions[MAX_STATES_PER_LAYER * MAX_TRANSITIONS_PER_STATE];
    int transition_count;
} sprite_anim_tree_data_t;

static sprite_anim_tree_data_t g_trees[MAX_TREES];
static int g_tree_count = 0;

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

static int tree_index(sprite_anim_tree_t tree)
{
    if (!tree) return -1;
    int idx = (intptr_t)tree;
    return (idx >= 0 && idx < g_tree_count) ? idx : -1;
}

static sprite_anim_tree_data_t *tree_get(sprite_anim_tree_t tree)
{
    int idx = tree_index(tree);
    return (idx < 0) ? NULL : &g_trees[idx];
}

static int layer_index(sprite_anim_layer_t layer)
{
    if (!layer) return -1;
    int idx = (intptr_t)layer;
    return idx;
}

static int state_index(sprite_anim_state_t state)
{
    if (!state) return -1;
    int idx = (intptr_t)state;
    return idx;
}

static int clip_index(sprite_anim_clip_t clip)
{
    if (!clip) return -1;
    int idx = (intptr_t)clip;
    return idx;
}

static int transition_index(sprite_anim_transition_t trans)
{
    if (!trans) return -1;
    int idx = (intptr_t)trans;
    return idx;
}

/* ============================================================================
 * TREE CREATION & CONTROL
 * ========================================================================== */

sprite_anim_tree_t sprite_anim_tree_create(void *root, int is_group)
{
    if (!root) return INVALID_ANIM_TREE;
    if (g_tree_count >= MAX_TREES) return INVALID_ANIM_TREE;

    int idx = g_tree_count;
    sprite_anim_tree_data_t *tree = &g_trees[idx];

    tree->root = root;
    tree->is_group = is_group;
    tree->layer_count = 0;
    tree->active_state_index = -1;
    tree->playback_time = 0.0f;
    tree->time_scale = 1.0f;
    tree->is_playing = 0;
    tree->callback = NULL;
    tree->transition_count = 0;

    g_tree_count++;
    return (sprite_anim_tree_t)(intptr_t)idx;
}

void sprite_anim_tree_destroy(sprite_anim_tree_t tree)
{
    int idx = tree_index(tree);
    if (idx < 0) return;

    sprite_anim_tree_data_t *t = &g_trees[idx];
    for (int i = 0; i < t->layer_count; i++) {
        anim_layer_t *layer = &t->layers[i];
        for (int j = 0; j < layer->state_count; j++) {
            anim_state_t *state = &layer->states[j];
            for (int k = 0; k < state->clip_count; k++) {
                if (state->clips[k].frames) {
                    free(state->clips[k].frames);
                }
            }
        }
    }
    t->layer_count = 0;
}

int sprite_anim_tree_get_info(sprite_anim_tree_t tree,
                               sprite_anim_tree_info_t *info)
{
    if (!info) return 0;
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t) return 0;

    info->tree = tree;
    info->state_count = (t->layer_count > 0) ? t->layers[0].state_count : 0;
    info->active_state_index = t->active_state_index;
    info->playback_time = t->playback_time;
    info->time_scale = t->time_scale;
    info->is_playing = t->is_playing;
    return 1;
}

int sprite_anim_tree_update(sprite_anim_tree_t tree, int delta_ms)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || !t->is_playing) return 0;

    float delta_s = delta_ms / 1000.0f * t->time_scale;
    t->playback_time += delta_s;

    if (t->active_state_index >= 0 && t->layer_count > 0) {
        anim_layer_t *layer = &t->layers[0];
        if (t->active_state_index < layer->state_count) {
            anim_state_t *state = &layer->states[t->active_state_index];
            int duration = sprite_anim_state_get_duration((sprite_anim_state_t)(intptr_t)t->active_state_index);
            if (duration > 0 && t->playback_time >= duration / 1000.0f) {
                t->playback_time = 0.0f;
                if (t->callback) {
                    t->callback(tree, "state_end");
                }
            }
        }
    }

    int updated = 0;
    if (t->is_group) {
        sprite_group_t group = (sprite_group_t)t->root;
        updated = sprite_group_update_all(group, delta_ms);
    } else {
        sprite_t sprite = (sprite_t)t->root;
        updated = sprite_update_single(sprite, delta_ms);
    }

    return updated;
}

void sprite_anim_tree_set_time_scale(sprite_anim_tree_t tree, float scale)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) t->time_scale = scale;
}

float sprite_anim_tree_get_time_scale(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    return (t) ? t->time_scale : 1.0f;
}

void sprite_anim_tree_play(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) t->is_playing = 1;
}

void sprite_anim_tree_pause(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) t->is_playing = 0;
}

void sprite_anim_tree_stop(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) {
        t->is_playing = 0;
        t->playback_time = 0.0f;
    }
}

int sprite_anim_tree_is_playing(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    return (t) ? t->is_playing : 0;
}

/* ============================================================================
 * LAYER MANAGEMENT
 * ========================================================================== */

sprite_anim_layer_t sprite_anim_layer_add(sprite_anim_tree_t tree,
                                           const char *name, float weight,
                                           sprite_anim_layer_type_t type)
{
    if (!name) return INVALID_ANIM_LAYER;
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || t->layer_count >= MAX_LAYERS_PER_TREE) return INVALID_ANIM_LAYER;

    int idx = t->layer_count;
    anim_layer_t *layer = &t->layers[idx];

    strncpy(layer->name, name, MAX_NAME_LEN - 1);
    layer->weight = weight;
    layer->type = type;
    layer->state_count = 0;

    t->layer_count++;
    return (sprite_anim_layer_t)(intptr_t)idx;
}

float sprite_anim_layer_get_weight(sprite_anim_layer_t layer)
{
    int idx = layer_index(layer);
    if (idx < 0) return 0.0f;
    for (int i = 0; i < g_tree_count; i++) {
        if (idx < g_trees[i].layer_count) {
            return g_trees[i].layers[idx].weight;
        }
    }
    return 0.0f;
}

void sprite_anim_layer_set_weight(sprite_anim_layer_t layer, float weight)
{
    int idx = layer_index(layer);
    if (idx < 0) return;
    for (int i = 0; i < g_tree_count; i++) {
        if (idx < g_trees[i].layer_count) {
            g_trees[i].layers[idx].weight = weight;
            return;
        }
    }
}

int sprite_anim_layer_get_count(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    return (t) ? t->layer_count : 0;
}

sprite_anim_layer_t sprite_anim_layer_get(sprite_anim_tree_t tree, int index)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || index < 0 || index >= t->layer_count) return INVALID_ANIM_LAYER;
    return (sprite_anim_layer_t)(intptr_t)index;
}

/* ============================================================================
 * STATE MANAGEMENT
 * ========================================================================== */

sprite_anim_state_t sprite_anim_state_add(sprite_anim_layer_t layer,
                                           const char *name)
{
    if (!name) return INVALID_ANIM_STATE;
    int layer_idx = layer_index(layer);
    if (layer_idx < 0) return INVALID_ANIM_STATE;

    for (int i = 0; i < g_tree_count; i++) {
        if (layer_idx < g_trees[i].layer_count) {
            anim_layer_t *l = &g_trees[i].layers[layer_idx];
            if (l->state_count >= MAX_STATES_PER_LAYER) return INVALID_ANIM_STATE;

            int state_idx = l->state_count;
            anim_state_t *state = &l->states[state_idx];

            strncpy(state->name, name, MAX_NAME_LEN - 1);
            state->clip_count = 0;

            l->state_count++;
            return (sprite_anim_state_t)(intptr_t)state_idx;
        }
    }
    return INVALID_ANIM_STATE;
}

sprite_anim_clip_t sprite_anim_state_add_clip(sprite_anim_state_t state,
                                               uint8_t **frames,
                                               int frame_count,
                                               int frame_delay)
{
    if (!frames || frame_count <= 0) return INVALID_ANIM_CLIP;
    int state_idx = state_index(state);
    if (state_idx < 0) return INVALID_ANIM_CLIP;

    for (int i = 0; i < g_tree_count; i++) {
        for (int j = 0; j < g_trees[i].layer_count; j++) {
            anim_layer_t *l = &g_trees[i].layers[j];
            if (state_idx < l->state_count) {
                anim_state_t *s = &l->states[state_idx];
                if (s->clip_count >= MAX_CLIPS_PER_STATE) return INVALID_ANIM_CLIP;

                int clip_idx = s->clip_count;
                anim_clip_t *clip = &s->clips[clip_idx];

                clip->frames = frames;
                clip->frame_count = frame_count;
                clip->frame_delay = frame_delay;
                clip->total_duration = frame_count * frame_delay;

                s->clip_count++;
                return (sprite_anim_clip_t)(intptr_t)clip_idx;
            }
        }
    }
    return INVALID_ANIM_CLIP;
}

int sprite_anim_state_get_count(sprite_anim_layer_t layer)
{
    int layer_idx = layer_index(layer);
    if (layer_idx < 0) return 0;

    for (int i = 0; i < g_tree_count; i++) {
        if (layer_idx < g_trees[i].layer_count) {
            return g_trees[i].layers[layer_idx].state_count;
        }
    }
    return 0;
}

sprite_anim_state_t sprite_anim_state_get(sprite_anim_layer_t layer, int index)
{
    int layer_idx = layer_index(layer);
    if (layer_idx < 0) return INVALID_ANIM_STATE;

    for (int i = 0; i < g_tree_count; i++) {
        if (layer_idx < g_trees[i].layer_count) {
            anim_layer_t *l = &g_trees[i].layers[layer_idx];
            if (index >= 0 && index < l->state_count) {
                return (sprite_anim_state_t)(intptr_t)index;
            }
        }
    }
    return INVALID_ANIM_STATE;
}

sprite_anim_state_t sprite_anim_state_find(sprite_anim_layer_t layer,
                                            const char *name)
{
    if (!name) return INVALID_ANIM_STATE;
    int layer_idx = layer_index(layer);
    if (layer_idx < 0) return INVALID_ANIM_STATE;

    for (int i = 0; i < g_tree_count; i++) {
        if (layer_idx < g_trees[i].layer_count) {
            anim_layer_t *l = &g_trees[i].layers[layer_idx];
            for (int j = 0; j < l->state_count; j++) {
                if (strcmp(l->states[j].name, name) == 0) {
                    return (sprite_anim_state_t)(intptr_t)j;
                }
            }
        }
    }
    return INVALID_ANIM_STATE;
}

/* ============================================================================
 * STATE PLAYBACK
 * ========================================================================== */

int sprite_anim_tree_play_state(sprite_anim_tree_t tree,
                                 const char *state_name,
                                 sprite_anim_transition_type_t transition_type)
{
    if (!state_name) return 0;
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || t->layer_count == 0) return 0;

    anim_layer_t *layer = &t->layers[0];
    for (int i = 0; i < layer->state_count; i++) {
        if (strcmp(layer->states[i].name, state_name) == 0) {
            t->active_state_index = i;
            t->playback_time = 0.0f;
            t->is_playing = 1;
            if (t->callback) {
                t->callback(tree, "state_start");
            }
            return 1;
        }
    }
    return 0;
}

int sprite_anim_tree_queue_state(sprite_anim_tree_t tree,
                                  const char *state_name)
{
    return sprite_anim_tree_play_state(tree, state_name, ANIM_TRANSITION_QUEUE);
}

char *sprite_anim_tree_get_current_state(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || t->active_state_index < 0 || t->layer_count == 0) return NULL;

    anim_layer_t *layer = &t->layers[0];
    if (t->active_state_index >= layer->state_count) return NULL;

    char *result = malloc(MAX_NAME_LEN);
    if (result) {
        strncpy(result, layer->states[t->active_state_index].name, MAX_NAME_LEN - 1);
    }
    return result;
}

int sprite_anim_tree_is_state_finished(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t || t->active_state_index < 0 || t->layer_count == 0) return 0;

    anim_layer_t *layer = &t->layers[0];
    if (t->active_state_index >= layer->state_count) return 0;

    int duration = sprite_anim_state_get_duration((sprite_anim_state_t)(intptr_t)t->active_state_index);
    return (duration > 0 && t->playback_time >= duration / 1000.0f) ? 1 : 0;
}

/* ============================================================================
 * TRANSITIONS
 * ========================================================================== */

sprite_anim_transition_t sprite_anim_transition_add(sprite_anim_state_t from_state,
                                                     sprite_anim_state_t to_state,
                                                     const char *condition,
                                                     sprite_anim_transition_type_t type)
{
    if (!condition) return INVALID_ANIM_TRANSITION;

    for (int i = 0; i < g_tree_count; i++) {
        if (g_trees[i].transition_count >= MAX_STATES_PER_LAYER * MAX_TRANSITIONS_PER_STATE) {
            continue;
        }
        int idx = g_trees[i].transition_count;
        anim_transition_t *trans = &g_trees[i].transitions[idx];

        strncpy(trans->condition, condition, MAX_NAME_LEN - 1);
        trans->type = type;
        trans->active = 1;

        g_trees[i].transition_count++;
        return (sprite_anim_transition_t)(intptr_t)idx;
    }
    return INVALID_ANIM_TRANSITION;
}

int sprite_anim_tree_trigger_transition(sprite_anim_tree_t tree,
                                         const char *condition)
{
    if (!condition) return 0;
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t) return 0;

    for (int i = 0; i < t->transition_count; i++) {
        if (strcmp(t->transitions[i].condition, condition) == 0) {
            if (t->callback) {
                t->callback(tree, condition);
            }
            return 1;
        }
    }
    return 0;
}

void sprite_anim_transition_remove(sprite_anim_transition_t transition)
{
    int idx = transition_index(transition);
    if (idx < 0) return;

    for (int i = 0; i < g_tree_count; i++) {
        if (idx < g_trees[i].transition_count) {
            g_trees[i].transitions[idx].active = 0;
            return;
        }
    }
}

/* ============================================================================
 * CALLBACKS & EVENTS
 * ========================================================================== */

void sprite_anim_tree_set_callback(sprite_anim_tree_t tree,
                                    sprite_anim_callback callback)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) t->callback = callback;
}

/* ============================================================================
 * SYNCHRONIZATION
 * ========================================================================== */

void sprite_anim_tree_synchronize_children(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t) return;

    if (t->is_group) {
        sprite_group_t group = (sprite_group_t)t->root;
        int count = sprite_group_get_member_count(group);
        for (int i = 0; i < count; i++) {
            sprite_t child = sprite_group_get_member(group, i);
            if (child) {
                sprite_update_single(child, 0);
            }
        }
    }
}

int sprite_anim_tree_get_child_trees(sprite_anim_tree_t tree,
                                      sprite_anim_tree_t *children,
                                      int max_children)
{
    if (!children || max_children <= 0) return 0;
    int count = 0;
    for (int i = 0; i < g_tree_count; i++) {
        if (count >= max_children) break;
        if (g_trees[i].root) {
            children[count++] = (sprite_anim_tree_t)(intptr_t)i;
        }
    }
    return count;
}

/* ============================================================================
 * QUERYING & INSPECTION
 * ========================================================================== */

int sprite_anim_tree_get_playback_time(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    return (t) ? (int)(t->playback_time * 1000.0f) : 0;
}

void sprite_anim_tree_set_playback_time(sprite_anim_tree_t tree, int time_ms)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (t) t->playback_time = time_ms / 1000.0f;
}

int sprite_anim_state_get_duration(sprite_anim_state_t state)
{
    int state_idx = state_index(state);
    if (state_idx < 0) return 0;

    for (int i = 0; i < g_tree_count; i++) {
        for (int j = 0; j < g_trees[i].layer_count; j++) {
            anim_layer_t *l = &g_trees[i].layers[j];
            if (state_idx < l->state_count) {
                anim_state_t *s = &l->states[state_idx];
                int total = 0;
                for (int k = 0; k < s->clip_count; k++) {
                    total += s->clips[k].total_duration;
                }
                return total;
            }
        }
    }
    return 0;
}

void sprite_anim_tree_print_state(sprite_anim_tree_t tree)
{
    sprite_anim_tree_info_t info;
    if (!sprite_anim_tree_get_info(tree, &info)) {
        printf("Animation tree: invalid\n");
        return;
    }

    printf("Animation Tree: states=%d active=%d time=%.2fs scale=%.2f playing=%d\n",
           info.state_count, info.active_state_index,
           info.playback_time, info.time_scale, info.is_playing);
}

void sprite_anim_tree_print_hierarchy(sprite_anim_tree_t tree)
{
    sprite_anim_tree_data_t *t = tree_get(tree);
    if (!t) {
        printf("Animation tree: invalid\n");
        return;
    }

    printf("Animation Tree Hierarchy:\n");
    for (int i = 0; i < t->layer_count; i++) {
        anim_layer_t *layer = &t->layers[i];
        printf("  Layer %d: %s (weight=%.2f, type=%d)\n", i, layer->name,
               layer->weight, layer->type);
        for (int j = 0; j < layer->state_count; j++) {
            anim_state_t *state = &layer->states[j];
            printf("    State %d: %s (clips=%d)\n", j, state->name,
                   state->clip_count);
        }
    }
}
