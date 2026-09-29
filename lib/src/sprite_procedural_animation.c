/* sprite_procedural_animation.c — Procedural Animation Implementation */

#include "sprite_procedural_animation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_LOCOMOTION_GENS 8
#define MAX_IDLE_GENS 8
#define MAX_REACTIVE_GENS 8

typedef struct {
    sprite_skeleton_t skeleton;
    float stride_length;
    float cadence;
    float direction_x, direction_y;
    float speed;
} locomotion_gen_impl;

typedef struct {
    sprite_skeleton_t skeleton;
    char variations[16][64];
    int variation_count;
} idle_gen_impl;

typedef struct {
    sprite_skeleton_t skeleton;
    float recovery_time;
} reactive_gen_impl;

static locomotion_gen_impl locomotion_gens[MAX_LOCOMOTION_GENS];
static int locomotion_gen_count = 0;

static idle_gen_impl idle_gens[MAX_IDLE_GENS];
static int idle_gen_count = 0;

static reactive_gen_impl reactive_gens[MAX_REACTIVE_GENS];
static int reactive_gen_count = 0;

sprite_locomotion_generator_t sprite_locomotion_generator_create(sprite_skeleton_t skeleton) {
    if (locomotion_gen_count >= MAX_LOCOMOTION_GENS) return INVALID_LOCOMOTION;

    locomotion_gen_impl *gen = &locomotion_gens[locomotion_gen_count];
    gen->skeleton = skeleton;
    gen->stride_length = 1.0f;
    gen->cadence = 1.0f;
    gen->direction_x = 1.0f;
    gen->direction_y = 0.0f;
    gen->speed = 1.0f;

    return (sprite_locomotion_generator_t)(intptr_t)locomotion_gen_count++;
}

void sprite_locomotion_generator_destroy(sprite_locomotion_generator_t gen) {}

sprite_anim_tree_t sprite_locomotion_generator_generate(sprite_locomotion_generator_t gen,
                                                        sprite_locomotion_type_t type,
                                                        float speed) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= locomotion_gen_count) return NULL;

    locomotion_gen_impl *g = &locomotion_gens[idx];
    g->speed = speed;

    sprite_anim_tree_t tree = sprite_anim_tree_create(NULL, 0);

    float step_duration = 500.0f / (speed * g->cadence);

    switch (type) {
        case LOCOMOTION_WALK: {
            sprite_anim_layer_t layer = sprite_anim_tree_add_layer(tree);
            sprite_anim_state_t state = sprite_anim_tree_add_state(tree, layer, "walk");
            break;
        }
        case LOCOMOTION_RUN: {
            sprite_anim_layer_t layer = sprite_anim_tree_add_layer(tree);
            sprite_anim_state_t state = sprite_anim_tree_add_state(tree, layer, "run");
            break;
        }
        case LOCOMOTION_SPRINT: {
            sprite_anim_layer_t layer = sprite_anim_tree_add_layer(tree);
            sprite_anim_state_t state = sprite_anim_tree_add_state(tree, layer, "sprint");
            break;
        }
        default:
            break;
    }

    return tree;
}

int sprite_locomotion_generator_set_stride_length(sprite_locomotion_generator_t gen,
                                                   float stride) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= locomotion_gen_count) return 0;
    locomotion_gens[idx].stride_length = stride;
    return 1;
}

int sprite_locomotion_generator_set_cadence(sprite_locomotion_generator_t gen,
                                             float cadence) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= locomotion_gen_count) return 0;
    locomotion_gens[idx].cadence = cadence;
    return 1;
}

sprite_idle_generator_t sprite_idle_generator_create(sprite_skeleton_t skeleton) {
    if (idle_gen_count >= MAX_IDLE_GENS) return INVALID_IDLE;

    idle_gen_impl *gen = &idle_gens[idle_gen_count];
    gen->skeleton = skeleton;
    gen->variation_count = 0;

    return (sprite_idle_generator_t)(intptr_t)idle_gen_count++;
}

void sprite_idle_generator_destroy(sprite_idle_generator_t gen) {}

sprite_anim_tree_t sprite_idle_generator_generate(sprite_idle_generator_t gen,
                                                  sprite_idle_type_t type,
                                                  int variation_seed) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= idle_gen_count) return NULL;

    idle_gen_impl *g = &idle_gens[idx];
    srand(variation_seed);

    sprite_anim_tree_t tree = sprite_anim_tree_create(NULL, 0);

    const char *idle_names[] = {"stand", "sway", "breathe", "fidget"};
    const char *state_name = idle_names[type % 4];

    sprite_anim_layer_t layer = sprite_anim_tree_add_layer(tree);
    sprite_anim_state_t state = sprite_anim_tree_add_state(tree, layer, state_name);

    return tree;
}

int sprite_idle_generator_add_variation(sprite_idle_generator_t gen,
                                        const char *variation_name) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= idle_gen_count) return 0;

    idle_gen_impl *g = &idle_gens[idx];
    if (g->variation_count >= 16) return 0;

    strncpy(g->variations[g->variation_count], variation_name, 63);
    return ++g->variation_count;
}

int sprite_idle_generator_get_variation_count(sprite_idle_generator_t gen) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= idle_gen_count) return 0;
    return idle_gens[idx].variation_count;
}

sprite_reactive_generator_t sprite_reactive_generator_create(sprite_skeleton_t skeleton) {
    if (reactive_gen_count >= MAX_REACTIVE_GENS) return INVALID_REACTIVE;

    reactive_gen_impl *gen = &reactive_gens[reactive_gen_count];
    gen->skeleton = skeleton;
    gen->recovery_time = 1.0f;

    return (sprite_reactive_generator_t)(intptr_t)reactive_gen_count++;
}

void sprite_reactive_generator_destroy(sprite_reactive_generator_t gen) {}

sprite_anim_tree_t sprite_reactive_generator_generate(sprite_reactive_generator_t gen,
                                                      sprite_reactive_type_t type,
                                                      float intensity) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= reactive_gen_count) return NULL;

    reactive_gen_impl *g = &reactive_gens[idx];

    sprite_anim_tree_t tree = sprite_anim_tree_create(NULL, 0);

    const char *reactive_names[] = {"stumble", "fall", "impact", "slide"};
    const char *state_name = reactive_names[type % 4];

    float duration = 200.0f * (1.0f + intensity);

    sprite_anim_layer_t layer = sprite_anim_tree_add_layer(tree);
    sprite_anim_state_t state = sprite_anim_tree_add_state(tree, layer, state_name);

    return tree;
}

int sprite_reactive_generator_set_recovery_time(sprite_reactive_generator_t gen,
                                                float recovery_time) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= reactive_gen_count) return 0;
    reactive_gens[idx].recovery_time = recovery_time;
    return 1;
}

int sprite_locomotion_set_direction(sprite_locomotion_generator_t gen,
                                     float direction_x, float direction_y) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= locomotion_gen_count) return 0;

    locomotion_gen_impl *g = &locomotion_gens[idx];
    float len = sqrtf(direction_x * direction_x + direction_y * direction_y);
    if (len > 0.0f) {
        g->direction_x = direction_x / len;
        g->direction_y = direction_y / len;
    }
    return 1;
}

int sprite_locomotion_set_speed(sprite_locomotion_generator_t gen, float speed) {
    intptr_t idx = (intptr_t)gen;
    if (idx < 0 || idx >= locomotion_gen_count) return 0;
    locomotion_gens[idx].speed = speed;
    return 1;
}
