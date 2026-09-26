/* sprite_procedural_rigging.c — Procedural Rigging Implementation */

#include "sprite_procedural_rigging.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_GENERATORS 8
#define MAX_BONE_WEIGHTS 16

typedef struct {
    sprite_rig_type_t type;
    sprite_skeleton_t skeleton;
    int bone_count;
    int width, height;
    int seed;
} rig_generator_impl;

typedef struct {
    sprite_skeleton_t skeleton;
    float weights[256][64];
} bone_weight_impl;

static rig_generator_impl generators[MAX_GENERATORS];
static int generator_count = 0;

static bone_weight_impl bone_weights[MAX_BONE_WEIGHTS];
static int bone_weight_count = 0;

static void generate_biped_skeleton(rig_generator_impl *gen, int width, int height) {
    gen->skeleton = sprite_skeleton_create();

    int mid_x = width / 2;
    int base_y = height - 20;

    sprite_bone_t root = sprite_skeleton_add_bone(gen->skeleton, "root", NULL);
    sprite_skeleton_set_bone_position(gen->skeleton, root, mid_x, base_y);

    sprite_bone_t spine = sprite_skeleton_add_bone(gen->skeleton, "spine", root);
    sprite_skeleton_set_bone_position(gen->skeleton, spine, mid_x, base_y - 20);

    sprite_bone_t chest = sprite_skeleton_add_bone(gen->skeleton, "chest", spine);
    sprite_skeleton_set_bone_position(gen->skeleton, chest, mid_x, base_y - 40);

    sprite_bone_t head = sprite_skeleton_add_bone(gen->skeleton, "head", chest);
    sprite_skeleton_set_bone_position(gen->skeleton, head, mid_x, base_y - 60);

    sprite_bone_t left_arm = sprite_skeleton_add_bone(gen->skeleton, "left_arm", chest);
    sprite_skeleton_set_bone_position(gen->skeleton, left_arm, mid_x - 15, base_y - 45);

    sprite_bone_t left_forearm = sprite_skeleton_add_bone(gen->skeleton, "left_forearm", left_arm);
    sprite_skeleton_set_bone_position(gen->skeleton, left_forearm, mid_x - 30, base_y - 30);

    sprite_bone_t right_arm = sprite_skeleton_add_bone(gen->skeleton, "right_arm", chest);
    sprite_skeleton_set_bone_position(gen->skeleton, right_arm, mid_x + 15, base_y - 45);

    sprite_bone_t right_forearm = sprite_skeleton_add_bone(gen->skeleton, "right_forearm", right_arm);
    sprite_skeleton_set_bone_position(gen->skeleton, right_forearm, mid_x + 30, base_y - 30);

    sprite_bone_t left_leg = sprite_skeleton_add_bone(gen->skeleton, "left_leg", root);
    sprite_skeleton_set_bone_position(gen->skeleton, left_leg, mid_x - 8, base_y);

    sprite_bone_t left_foot = sprite_skeleton_add_bone(gen->skeleton, "left_foot", left_leg);
    sprite_skeleton_set_bone_position(gen->skeleton, left_foot, mid_x - 8, base_y + 20);

    sprite_bone_t right_leg = sprite_skeleton_add_bone(gen->skeleton, "right_leg", root);
    sprite_skeleton_set_bone_position(gen->skeleton, right_leg, mid_x + 8, base_y);

    sprite_bone_t right_foot = sprite_skeleton_add_bone(gen->skeleton, "right_foot", right_leg);
    sprite_skeleton_set_bone_position(gen->skeleton, right_foot, mid_x + 8, base_y + 20);

    gen->bone_count = 13;
}

static void generate_quadruped_skeleton(rig_generator_impl *gen, int width, int height) {
    gen->skeleton = sprite_skeleton_create();

    int mid_x = width / 2;
    int mid_y = height / 2;

    sprite_bone_t root = sprite_skeleton_add_bone(gen->skeleton, "root", NULL);
    sprite_skeleton_set_bone_position(gen->skeleton, root, mid_x, mid_y);

    sprite_bone_t spine = sprite_skeleton_add_bone(gen->skeleton, "spine", root);
    sprite_skeleton_set_bone_position(gen->skeleton, spine, mid_x, mid_y - 10);

    sprite_bone_t front_left = sprite_skeleton_add_bone(gen->skeleton, "front_left", spine);
    sprite_skeleton_set_bone_position(gen->skeleton, front_left, mid_x - 20, mid_y);

    sprite_bone_t front_right = sprite_skeleton_add_bone(gen->skeleton, "front_right", spine);
    sprite_skeleton_set_bone_position(gen->skeleton, front_right, mid_x + 20, mid_y);

    sprite_bone_t back_left = sprite_skeleton_add_bone(gen->skeleton, "back_left", root);
    sprite_skeleton_set_bone_position(gen->skeleton, back_left, mid_x - 15, mid_y + 10);

    sprite_bone_t back_right = sprite_skeleton_add_bone(gen->skeleton, "back_right", root);
    sprite_skeleton_set_bone_position(gen->skeleton, back_right, mid_x + 15, mid_y + 10);

    sprite_bone_t head = sprite_skeleton_add_bone(gen->skeleton, "head", spine);
    sprite_skeleton_set_bone_position(gen->skeleton, head, mid_x - 30, mid_y - 15);

    gen->bone_count = 7;
}

sprite_rig_generator_t sprite_rig_generator_create(sprite_rig_type_t type) {
    if (generator_count >= MAX_GENERATORS) return INVALID_RIG_GENERATOR;
    rig_generator_impl *gen = &generators[generator_count];
    gen->type = type;
    gen->skeleton = NULL;
    gen->bone_count = 0;
    gen->width = gen->height = 0;
    gen->seed = 0;
    return (sprite_rig_generator_t)(intptr_t)generator_count++;
}

void sprite_rig_generator_destroy(sprite_rig_generator_t generator) {
    intptr_t idx = (intptr_t)generator;
    if (idx < 0 || idx >= generator_count) return;
    if (generators[idx].skeleton) {
        sprite_skeleton_destroy(generators[idx].skeleton);
    }
}

sprite_skeleton_t sprite_rig_generator_generate(sprite_rig_generator_t generator,
                                                int width, int height, int seed) {
    intptr_t idx = (intptr_t)generator;
    if (idx < 0 || idx >= generator_count) return NULL;

    rig_generator_impl *gen = &generators[idx];
    gen->width = width;
    gen->height = height;
    gen->seed = seed;

    switch (gen->type) {
        case RIG_BIPED:
            generate_biped_skeleton(gen, width, height);
            break;
        case RIG_QUADRUPED:
            generate_quadruped_skeleton(gen, width, height);
            break;
        case RIG_CUSTOM:
            gen->skeleton = sprite_skeleton_create();
            break;
    }

    return gen->skeleton;
}

int sprite_rig_generator_set_bone_count(sprite_rig_generator_t generator, int count) {
    intptr_t idx = (intptr_t)generator;
    if (idx < 0 || idx >= generator_count) return 0;
    generators[idx].bone_count = count;
    return 1;
}

int sprite_rig_generator_optimize_placement(sprite_rig_generator_t generator) {
    intptr_t idx = (intptr_t)generator;
    if (idx < 0 || idx >= generator_count) return 0;
    return 1;
}

sprite_bone_weight_t sprite_bone_weight_create(sprite_skeleton_t skeleton) {
    if (bone_weight_count >= MAX_BONE_WEIGHTS) return INVALID_BONE_WEIGHT;
    bone_weight_impl *bw = &bone_weights[bone_weight_count];
    bw->skeleton = skeleton;
    memset(bw->weights, 0, sizeof(bw->weights));
    return (sprite_bone_weight_t)(intptr_t)bone_weight_count++;
}

void sprite_bone_weight_destroy(sprite_bone_weight_t weights) {}

void sprite_bone_weight_set_influence(sprite_bone_weight_t weights, int x, int y,
                                      sprite_bone_t bone, float weight) {
    intptr_t idx = (intptr_t)weights;
    if (idx < 0 || idx >= bone_weight_count) return;
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return;

    intptr_t bone_idx = (intptr_t)bone;
    if (bone_idx >= 0 && bone_idx < 64) {
        bone_weights[idx].weights[x][bone_idx] = weight;
    }
}

float sprite_bone_weight_get_influence(sprite_bone_weight_t weights, int x, int y,
                                       sprite_bone_t bone) {
    intptr_t idx = (intptr_t)weights;
    if (idx < 0 || idx >= bone_weight_count) return 0.0f;
    if (x < 0 || x >= 256 || y < 0 || y >= 64) return 0.0f;

    intptr_t bone_idx = (intptr_t)bone;
    if (bone_idx >= 0 && bone_idx < 64) {
        return bone_weights[idx].weights[x][bone_idx];
    }
    return 0.0f;
}

int sprite_bone_weight_compute_deformation(sprite_bone_weight_t weights, int x, int y,
                                           float *out_x, float *out_y) {
    intptr_t idx = (intptr_t)weights;
    if (idx < 0 || idx >= bone_weight_count || !out_x || !out_y) return 0;

    *out_x = (float)x;
    *out_y = (float)y;
    return 1;
}

int sprite_rig_generator_load_template(sprite_rig_generator_t generator,
                                       const char *template_name) {
    return 1;
}

int sprite_rig_generator_save_template(sprite_rig_generator_t generator,
                                       const char *template_name) {
    return 1;
}
