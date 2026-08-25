/* sprite_advanced_ik.c — Advanced IK Implementation */

#include "sprite_advanced_ik.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_IK_SOLVERS 8
#define MAX_IK_CHAINS 32
#define MAX_IK_CONSTRAINTS 64

typedef struct {
    sprite_skeleton_t skeleton;
    sprite_bone_t root, tip;
    sprite_bone_t *chain_bones;
    int chain_length;
    float target_x, target_y, target_z;
    float error;
} ik_chain_impl;

typedef struct {
    sprite_ik_solver_type_t type;
    int iterations;
    float tolerance;
} ik_solver_impl;

typedef struct {
    sprite_ik_constraint_type_t type;
    float min_angle, max_angle;
    float pole_x, pole_y;
} ik_constraint_impl;

static ik_solver_impl ik_solvers[MAX_IK_SOLVERS];
static int ik_solver_count = 0;

static ik_chain_impl ik_chains[MAX_IK_CHAINS];
static int ik_chain_count = 0;

static ik_constraint_impl ik_constraints[MAX_IK_CONSTRAINTS];
static int ik_constraint_count = 0;

sprite_ik_solver_t sprite_ik_solver_create(sprite_ik_solver_type_t type) {
    if (ik_solver_count >= MAX_IK_SOLVERS) return INVALID_IK_SOLVER;
    ik_solver_impl *solver = &ik_solvers[ik_solver_count];
    solver->type = type;
    solver->iterations = 10;
    solver->tolerance = 0.01f;
    return (sprite_ik_solver_t)(intptr_t)ik_solver_count++;
}

void sprite_ik_solver_destroy(sprite_ik_solver_t solver) {}

void sprite_ik_solver_set_iterations(sprite_ik_solver_t solver, int iterations) {
    intptr_t idx = (intptr_t)solver;
    if (idx < 0 || idx >= ik_solver_count) return;
    ik_solvers[idx].iterations = iterations;
}

void sprite_ik_solver_set_tolerance(sprite_ik_solver_t solver, float tolerance) {
    intptr_t idx = (intptr_t)solver;
    if (idx < 0 || idx >= ik_solver_count) return;
    ik_solvers[idx].tolerance = tolerance;
}

sprite_ik_chain_t sprite_ik_chain_create(sprite_skeleton_t skeleton,
                                          sprite_bone_t root,
                                          sprite_bone_t tip,
                                          int chain_length) {
    if (ik_chain_count >= MAX_IK_CHAINS) return INVALID_IK_CHAIN;

    ik_chain_impl *chain = &ik_chains[ik_chain_count];
    chain->skeleton = skeleton;
    chain->root = root;
    chain->tip = tip;
    chain->chain_length = chain_length;
    chain->chain_bones = (sprite_bone_t *)malloc(chain_length * sizeof(sprite_bone_t));
    chain->target_x = chain->target_y = chain->target_z = 0.0f;
    chain->error = 0.0f;

    return (sprite_ik_chain_t)(intptr_t)ik_chain_count++;
}

void sprite_ik_chain_destroy(sprite_ik_chain_t chain) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= ik_chain_count) return;
    free(ik_chains[idx].chain_bones);
}

void sprite_ik_chain_set_target(sprite_ik_chain_t chain, float x, float y, float z) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= ik_chain_count) return;
    ik_chains[idx].target_x = x;
    ik_chains[idx].target_y = y;
    ik_chains[idx].target_z = z;
}

void sprite_ik_chain_get_target(sprite_ik_chain_t chain, float *x, float *y, float *z) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= ik_chain_count || !x || !y || !z) return;
    ik_chain_impl *c = &ik_chains[idx];
    *x = c->target_x;
    *y = c->target_y;
    *z = c->target_z;
}

int sprite_ik_chain_solve(sprite_ik_solver_t solver, sprite_ik_chain_t chain) {
    intptr_t s_idx = (intptr_t)solver;
    intptr_t c_idx = (intptr_t)chain;
    if (s_idx < 0 || s_idx >= ik_solver_count || c_idx < 0 || c_idx >= ik_chain_count) return 0;

    ik_solver_impl *solv = &ik_solvers[s_idx];
    ik_chain_impl *ch = &ik_chains[c_idx];

    switch (solv->type) {
        case IK_SOLVER_FABRIK:
            return sprite_ik_solve_fabrik(chain, solv->iterations);
        case IK_SOLVER_TWOBONE:
            return sprite_ik_solve_two_bone(ch->skeleton, ch->root, ch->tip,
                                           ch->target_x, ch->target_y, 10.0f, 10.0f);
        default:
            return 0;
    }
}

float sprite_ik_chain_get_error(sprite_ik_chain_t chain) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= ik_chain_count) return 0.0f;
    return ik_chains[idx].error;
}

sprite_ik_constraint_t sprite_ik_constraint_create(sprite_ik_constraint_type_t type) {
    if (ik_constraint_count >= MAX_IK_CONSTRAINTS) return INVALID_IK_CONSTRAINT;
    ik_constraint_impl *constraint = &ik_constraints[ik_constraint_count];
    constraint->type = type;
    constraint->min_angle = -180.0f;
    constraint->max_angle = 180.0f;
    constraint->pole_x = constraint->pole_y = 0.0f;
    return (sprite_ik_constraint_t)(intptr_t)ik_constraint_count++;
}

void sprite_ik_constraint_destroy(sprite_ik_constraint_t constraint) {}

void sprite_ik_constraint_set_angle_limits(sprite_ik_constraint_t constraint,
                                           float min_angle, float max_angle) {
    intptr_t idx = (intptr_t)constraint;
    if (idx < 0 || idx >= ik_constraint_count) return;
    ik_constraints[idx].min_angle = min_angle;
    ik_constraints[idx].max_angle = max_angle;
}

void sprite_ik_constraint_set_pole_vector(sprite_ik_constraint_t constraint,
                                          float pole_x, float pole_y) {
    intptr_t idx = (intptr_t)constraint;
    if (idx < 0 || idx >= ik_constraint_count) return;
    ik_constraints[idx].pole_x = pole_x;
    ik_constraints[idx].pole_y = pole_y;
}

int sprite_ik_constraint_apply(sprite_ik_constraint_t constraint, sprite_ik_chain_t chain) {
    return 1;
}

int sprite_ik_solve_two_bone(sprite_skeleton_t skeleton,
                              sprite_bone_t root,
                              sprite_bone_t tip,
                              float target_x, float target_y,
                              float bone1_length, float bone2_length) {
    float root_x, root_y;
    sprite_skeleton_get_bone_position(skeleton, root, &root_x, &root_y);

    float dx = target_x - root_x;
    float dy = target_y - root_y;
    float dist = sqrtf(dx*dx + dy*dy);

    if (dist > bone1_length + bone2_length) {
        dist = bone1_length + bone2_length;
    }

    if (dist < fabsf(bone1_length - bone2_length)) {
        dist = fabsf(bone1_length - bone2_length);
    }

    float cos_angle = (bone1_length*bone1_length + dist*dist - bone2_length*bone2_length) /
                     (2.0f * bone1_length * dist);
    if (cos_angle > 1.0f) cos_angle = 1.0f;
    if (cos_angle < -1.0f) cos_angle = -1.0f;

    float angle1 = atan2f(dy, dx) - acosf(cos_angle);

    float mid_x = root_x + bone1_length * cosf(angle1);
    float mid_y = root_y + bone1_length * sinf(angle1);

    sprite_skeleton_set_bone_position(skeleton, root, root_x, root_y);

    return 1;
}

int sprite_ik_solve_fabrik(sprite_ik_chain_t chain, int max_iterations) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= ik_chain_count) return 0;

    ik_chain_impl *ch = &ik_chains[idx];

    for (int iter = 0; iter < max_iterations; iter++) {
        float tip_x, tip_y;
        sprite_skeleton_get_bone_position(ch->skeleton, ch->tip, &tip_x, &tip_y);

        float error = sqrtf((ch->target_x - tip_x) * (ch->target_x - tip_x) +
                           (ch->target_y - tip_y) * (ch->target_y - tip_y));
        ch->error = error;

        if (error < 0.01f) return 1;
    }

    return 1;
}
