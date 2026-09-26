/* sprite_advanced_ik.h — Phase 44: Advanced IK Solvers
 *
 * Multi-bone IK with FABRIK, pole vectors, and constraints.
 */

#ifndef SPRITE_ADVANCED_IK_H
#define SPRITE_ADVANCED_IK_H

#include <sprite_skeletal_animation.h>

typedef void *sprite_ik_solver_t;
typedef void *sprite_ik_chain_t;
typedef void *sprite_ik_constraint_t;

#define INVALID_IK_SOLVER NULL
#define INVALID_IK_CHAIN NULL
#define INVALID_IK_CONSTRAINT NULL

typedef enum {
    IK_SOLVER_FABRIK = 0,
    IK_SOLVER_CCD = 1,
    IK_SOLVER_TWOBONE = 2,
} sprite_ik_solver_type_t;

typedef enum {
    IK_CONSTRAINT_ANGLE = 0,
    IK_CONSTRAINT_POLE_VECTOR = 1,
    IK_CONSTRAINT_DISTANCE = 2,
} sprite_ik_constraint_type_t;

/* IK Solver */
sprite_ik_solver_t sprite_ik_solver_create(sprite_ik_solver_type_t type);
void sprite_ik_solver_destroy(sprite_ik_solver_t solver);
void sprite_ik_solver_set_iterations(sprite_ik_solver_t solver, int iterations);
void sprite_ik_solver_set_tolerance(sprite_ik_solver_t solver, float tolerance);

/* IK Chain */
sprite_ik_chain_t sprite_ik_chain_create(sprite_skeleton_t skeleton,
                                          sprite_bone_t root,
                                          sprite_bone_t tip,
                                          int chain_length);
void sprite_ik_chain_destroy(sprite_ik_chain_t chain);
void sprite_ik_chain_set_target(sprite_ik_chain_t chain, float x, float y, float z);
void sprite_ik_chain_get_target(sprite_ik_chain_t chain, float *x, float *y, float *z);
int sprite_ik_chain_solve(sprite_ik_solver_t solver, sprite_ik_chain_t chain);
float sprite_ik_chain_get_error(sprite_ik_chain_t chain);

/* Constraints */
sprite_ik_constraint_t sprite_ik_constraint_create(sprite_ik_constraint_type_t type);
void sprite_ik_constraint_destroy(sprite_ik_constraint_t constraint);
void sprite_ik_constraint_set_angle_limits(sprite_ik_constraint_t constraint,
                                           float min_angle, float max_angle);
void sprite_ik_constraint_set_pole_vector(sprite_ik_constraint_t constraint,
                                          float pole_x, float pole_y);
int sprite_ik_constraint_apply(sprite_ik_constraint_t constraint, sprite_ik_chain_t chain);

/* Two-Bone IK */
int sprite_ik_solve_two_bone(sprite_skeleton_t skeleton,
                              sprite_bone_t root,
                              sprite_bone_t tip,
                              float target_x, float target_y,
                              float bone1_length, float bone2_length);

/* FABRIK Algorithm */
int sprite_ik_solve_fabrik(sprite_ik_chain_t chain, int max_iterations);

#endif
