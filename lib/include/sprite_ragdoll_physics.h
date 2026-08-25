/* sprite_ragdoll_physics.h — Ragdoll Physics for Skeletal Systems
 *
 * Provides soft-body physics simulation for skeletal animations.
 *
 * Features:
 * - Bone constraints with configurable stiffness
 * - Gravity and force simulation
 * - Collision detection between limbs
 * - Damping and energy loss
 * - Physics-driven bone positioning
 */

#ifndef SPRITE_RAGDOLL_PHYSICS_H
#define SPRITE_RAGDOLL_PHYSICS_H

#include <sprite_skeletal_animation.h>

typedef void *sprite_ragdoll_t;
typedef void *sprite_constraint_t;

#define INVALID_RAGDOLL NULL
#define INVALID_CONSTRAINT NULL

typedef enum {
    CONSTRAINT_DISTANCE = 0,
    CONSTRAINT_ANGLE = 1,
    CONSTRAINT_BALL_SOCKET = 2,
    CONSTRAINT_HINGE = 3,
} sprite_constraint_type_t;

typedef struct {
    sprite_ragdoll_t ragdoll;
    int bone_count;
    int constraint_count;
    float gravity_x, gravity_y;
    int is_active;
} sprite_ragdoll_info_t;

/* Creation & Control */
sprite_ragdoll_t sprite_ragdoll_create(sprite_skeleton_t skeleton);
void sprite_ragdoll_destroy(sprite_ragdoll_t ragdoll);
int sprite_ragdoll_get_info(sprite_ragdoll_t ragdoll, sprite_ragdoll_info_t *info);

/* Physics Parameters */
void sprite_ragdoll_set_gravity(sprite_ragdoll_t ragdoll, float gx, float gy);
void sprite_ragdoll_get_gravity(sprite_ragdoll_t ragdoll, float *gx, float *gy);
void sprite_ragdoll_set_damping(sprite_ragdoll_t ragdoll, float damping);
float sprite_ragdoll_get_damping(sprite_ragdoll_t ragdoll);

/* Bone Physics */
void sprite_ragdoll_set_bone_mass(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float mass);
float sprite_ragdoll_get_bone_mass(sprite_ragdoll_t ragdoll, sprite_bone_t bone);
void sprite_ragdoll_set_bone_velocity(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float vx, float vy);
void sprite_ragdoll_get_bone_velocity(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float *vx, float *vy);
void sprite_ragdoll_apply_force(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float fx, float fy);
void sprite_ragdoll_apply_impulse(sprite_ragdoll_t ragdoll, sprite_bone_t bone, float ix, float iy);

/* Constraints */
sprite_constraint_t sprite_ragdoll_add_constraint(sprite_ragdoll_t ragdoll,
                                                    sprite_constraint_type_t type,
                                                    sprite_bone_t bone1,
                                                    sprite_bone_t bone2,
                                                    float param1,
                                                    float param2);
int sprite_ragdoll_remove_constraint(sprite_ragdoll_t ragdoll, sprite_constraint_t constraint);
int sprite_ragdoll_get_constraint_count(sprite_ragdoll_t ragdoll);

/* Simulation */
void sprite_ragdoll_activate(sprite_ragdoll_t ragdoll);
void sprite_ragdoll_deactivate(sprite_ragdoll_t ragdoll);
int sprite_ragdoll_is_active(sprite_ragdoll_t ragdoll);
int sprite_ragdoll_update(sprite_ragdoll_t ragdoll, int delta_ms);

/* Collision */
int sprite_ragdoll_enable_self_collision(sprite_ragdoll_t ragdoll);
int sprite_ragdoll_disable_self_collision(sprite_ragdoll_t ragdoll);

/* Debug */
void sprite_ragdoll_print_state(sprite_ragdoll_t ragdoll);

#endif
