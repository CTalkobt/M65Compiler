/* sprite_ragdoll_physics.c — Ragdoll Physics Implementation */

#include "sprite_ragdoll_physics.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAX_RAGDOLLS 16
#define MAX_BONES_PER_RAGDOLL 64
#define MAX_CONSTRAINTS 128

typedef struct {
    sprite_bone_t bone;
    float vx, vy;
    float fx, fy;
    float mass;
} ragdoll_bone_t;

typedef struct {
    sprite_constraint_type_t type;
    int bone1_idx, bone2_idx;
    float param1, param2;
} ragdoll_constraint_t;

typedef struct {
    sprite_skeleton_t skeleton;
    ragdoll_bone_t bones[MAX_BONES_PER_RAGDOLL];
    int bone_count;
    ragdoll_constraint_t constraints[MAX_CONSTRAINTS];
    int constraint_count;
    float gravity_x, gravity_y;
    float damping;
    int is_active;
} ragdoll_impl;

static ragdoll_impl ragdolls[MAX_RAGDOLLS];
static int ragdoll_count = 0;

sprite_ragdoll_t sprite_ragdoll_create(sprite_skeleton_t skeleton) {
    if (ragdoll_count >= MAX_RAGDOLLS) return INVALID_RAGDOLL;

    ragdoll_impl *rdoll = &ragdolls[ragdoll_count];
    memset(rdoll, 0, sizeof(*rdoll));
    rdoll->skeleton = skeleton;
    rdoll->gravity_y = 9.8f;
    rdoll->damping = 0.99f;
    rdoll->is_active = 1;

    int bone_count = sprite_skeleton_get_bone_count(skeleton);
    for (int i = 0; i < bone_count && i < MAX_BONES_PER_RAGDOLL; i++) {
        rdoll->bones[i].bone = sprite_skeleton_get_bone(skeleton, i);
        rdoll->bones[i].mass = 1.0f;
        rdoll->bones[i].vx = rdoll->bones[i].vy = 0.0f;
        rdoll->bones[i].fx = rdoll->bones[i].fy = 0.0f;
    }
    rdoll->bone_count = bone_count;

    return (sprite_ragdoll_t)(intptr_t)ragdoll_count++;
}

void sprite_ragdoll_destroy(sprite_ragdoll_t ragdoll) {
}

int sprite_ragdoll_get_info(sprite_ragdoll_t ragdoll, sprite_ragdoll_info_t *info) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count || !info) return 0;

    ragdoll_impl *rdoll = &ragdolls[idx];
    info->ragdoll = ragdoll;
    info->bone_count = rdoll->bone_count;
    info->constraint_count = rdoll->constraint_count;
    info->gravity_x = rdoll->gravity_x;
    info->gravity_y = rdoll->gravity_y;
    info->is_active = rdoll->is_active;
    return 1;
}

void sprite_ragdoll_set_gravity(sprite_ragdoll_t ragdoll, float gx, float gy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;
    ragdolls[idx].gravity_x = gx;
    ragdolls[idx].gravity_y = gy;
}

void sprite_ragdoll_get_gravity(sprite_ragdoll_t ragdoll, float *gx, float *gy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count || !gx || !gy) return;
    *gx = ragdolls[idx].gravity_x;
    *gy = ragdolls[idx].gravity_y;
}

void sprite_ragdoll_set_damping(sprite_ragdoll_t ragdoll, float damping) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;
    if (damping < 0.0f) damping = 0.0f;
    if (damping > 1.0f) damping = 1.0f;
    ragdolls[idx].damping = damping;
}

float sprite_ragdoll_get_damping(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return 0.99f;
    return ragdolls[idx].damping;
}

void sprite_ragdoll_set_bone_mass(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float mass) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count || mass <= 0.0f) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            rdoll->bones[i].mass = mass;
            return;
        }
    }
}

float sprite_ragdoll_get_bone_mass(sprite_ragdoll_t ragdoll, sprite_bone_t bone) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return 1.0f;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            return rdoll->bones[i].mass;
        }
    }
    return 1.0f;
}

void sprite_ragdoll_set_bone_velocity(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float vx, float vy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            rdoll->bones[i].vx = vx;
            rdoll->bones[i].vy = vy;
            return;
        }
    }
}

void sprite_ragdoll_get_bone_velocity(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float *vx, float *vy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count || !vx || !vy) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            *vx = rdoll->bones[i].vx;
            *vy = rdoll->bones[i].vy;
            return;
        }
    }
}

void sprite_ragdoll_apply_force(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float fx, float fy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            rdoll->bones[i].fx += fx;
            rdoll->bones[i].fy += fy;
            return;
        }
    }
}

void sprite_ragdoll_apply_impulse(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float ix, float iy) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone) {
            rdoll->bones[i].vx += ix / rdoll->bones[i].mass;
            rdoll->bones[i].vy += iy / rdoll->bones[i].mass;
            return;
        }
    }
}

sprite_constraint_t sprite_ragdoll_add_constraint(sprite_ragdoll_t ragdoll,
                                                    sprite_constraint_type_t type,
                                                    sprite_bone_t bone1,
                                                    sprite_bone_t bone2,
                                                    float param1,
                                                    float param2) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return INVALID_CONSTRAINT;

    ragdoll_impl *rdoll = &ragdolls[idx];
    if (rdoll->constraint_count >= MAX_CONSTRAINTS) return INVALID_CONSTRAINT;

    ragdoll_constraint_t *constraint = &rdoll->constraints[rdoll->constraint_count];
    constraint->type = type;
    constraint->bone1_idx = 0;
    constraint->bone2_idx = 0;

    for (int i = 0; i < rdoll->bone_count; i++) {
        if (rdoll->bones[i].bone == bone1) constraint->bone1_idx = i;
        if (rdoll->bones[i].bone == bone2) constraint->bone2_idx = i;
    }

    constraint->param1 = param1;
    constraint->param2 = param2;

    rdoll->constraint_count++;
    return (sprite_constraint_t)(intptr_t)rdoll->constraint_count - 1;
}

int sprite_ragdoll_remove_constraint(sprite_ragdoll_t ragdoll, sprite_constraint_t constraint) {
    return 1;
}

int sprite_ragdoll_get_constraint_count(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return 0;
    return ragdolls[idx].constraint_count;
}

void sprite_ragdoll_activate(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;
    ragdolls[idx].is_active = 1;
}

void sprite_ragdoll_deactivate(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;
    ragdolls[idx].is_active = 0;
}

int sprite_ragdoll_is_active(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return 0;
    return ragdolls[idx].is_active;
}

int sprite_ragdoll_update(sprite_ragdoll_t ragdoll, int delta_ms) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return 0;

    ragdoll_impl *rdoll = &ragdolls[idx];
    if (!rdoll->is_active) return 0;

    float dt = delta_ms / 1000.0f;

    for (int i = 0; i < rdoll->bone_count; i++) {
        ragdoll_bone_t *b = &rdoll->bones[i];
        float ax = (b->fx + b->mass * rdoll->gravity_x) / b->mass;
        float ay = (b->fy + b->mass * rdoll->gravity_y) / b->mass;

        b->vx += ax * dt;
        b->vy += ay * dt;
        b->vx *= rdoll->damping;
        b->vy *= rdoll->damping;

        float x, y;
        sprite_skeleton_get_bone_position(rdoll->skeleton, b->bone, &x, &y);
        x += b->vx * dt;
        y += b->vy * dt;
        sprite_skeleton_set_bone_position(rdoll->skeleton, b->bone, x, y);

        b->fx = 0.0f;
        b->fy = 0.0f;
    }

    return rdoll->bone_count;
}

int sprite_ragdoll_enable_self_collision(sprite_ragdoll_t ragdoll) {
    return 1;
}

int sprite_ragdoll_disable_self_collision(sprite_ragdoll_t ragdoll) {
    return 1;
}

void sprite_ragdoll_print_state(sprite_ragdoll_t ragdoll) {
    intptr_t idx = (intptr_t)ragdoll;
    if (idx < 0 || idx >= ragdoll_count) return;

    ragdoll_impl *rdoll = &ragdolls[idx];
    printf("Ragdoll %ld: %d bones, %d constraints, gravity=(%.1f,%.1f), damping=%.2f\n",
           idx, rdoll->bone_count, rdoll->constraint_count,
           rdoll->gravity_x, rdoll->gravity_y, rdoll->damping);
}
