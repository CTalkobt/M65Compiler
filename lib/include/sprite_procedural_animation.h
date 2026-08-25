/* sprite_procedural_animation.h — Phase 45: Procedural Animation
 *
 * Procedural locomotion, idle variation, and reactive animations.
 */

#ifndef SPRITE_PROCEDURAL_ANIMATION_H
#define SPRITE_PROCEDURAL_ANIMATION_H

#include <sprite_skeletal_animation.h>

typedef void *sprite_locomotion_generator_t;
typedef void *sprite_idle_generator_t;
typedef void *sprite_reactive_generator_t;

#define INVALID_LOCOMOTION NULL
#define INVALID_IDLE NULL
#define INVALID_REACTIVE NULL

typedef enum {
    LOCOMOTION_WALK = 0,
    LOCOMOTION_RUN = 1,
    LOCOMOTION_SPRINT = 2,
    LOCOMOTION_STRAFE = 3,
    LOCOMOTION_CLIMB = 4,
} sprite_locomotion_type_t;

typedef enum {
    IDLE_STAND = 0,
    IDLE_SWAY = 1,
    IDLE_BREATHE = 2,
    IDLE_FIDGET = 3,
} sprite_idle_type_t;

typedef enum {
    REACTIVE_STUMBLE = 0,
    REACTIVE_FALL = 1,
    REACTIVE_IMPACT = 2,
    REACTIVE_SLIDE = 3,
} sprite_reactive_type_t;

/* Locomotion Generation */
sprite_locomotion_generator_t sprite_locomotion_generator_create(sprite_skeleton_t skeleton);
void sprite_locomotion_generator_destroy(sprite_locomotion_generator_t gen);
sprite_anim_tree_t sprite_locomotion_generator_generate(sprite_locomotion_generator_t gen,
                                                        sprite_locomotion_type_t type,
                                                        float speed);
int sprite_locomotion_generator_set_stride_length(sprite_locomotion_generator_t gen,
                                                   float stride);
int sprite_locomotion_generator_set_cadence(sprite_locomotion_generator_t gen,
                                             float cadence);

/* Idle Variation */
sprite_idle_generator_t sprite_idle_generator_create(sprite_skeleton_t skeleton);
void sprite_idle_generator_destroy(sprite_idle_generator_t gen);
sprite_anim_tree_t sprite_idle_generator_generate(sprite_idle_generator_t gen,
                                                  sprite_idle_type_t type,
                                                  int variation_seed);
int sprite_idle_generator_add_variation(sprite_idle_generator_t gen,
                                        const char *variation_name);
int sprite_idle_generator_get_variation_count(sprite_idle_generator_t gen);

/* Reactive Animations */
sprite_reactive_generator_t sprite_reactive_generator_create(sprite_skeleton_t skeleton);
void sprite_reactive_generator_destroy(sprite_reactive_generator_t gen);
sprite_anim_tree_t sprite_reactive_generator_generate(sprite_reactive_generator_t gen,
                                                      sprite_reactive_type_t type,
                                                      float intensity);
int sprite_reactive_generator_set_recovery_time(sprite_reactive_generator_t gen,
                                                float recovery_time);

/* Blending Locomotion with Direction */
int sprite_locomotion_set_direction(sprite_locomotion_generator_t gen,
                                     float direction_x, float direction_y);
int sprite_locomotion_set_speed(sprite_locomotion_generator_t gen, float speed);

#endif
