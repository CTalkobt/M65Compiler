/* FOR THE FUTURE: NOT IMPLEMENTED.
 *
 * No library defines what this header declares: a call of one of its
 * functions would compile and then fail to link. Its contents are kept for
 * later work and are left out unless CC45_FUTURE is defined (-DCC45_FUTURE),
 * so including it by accident declares nothing.
 * (doc/architecture/stdlib.md, "Headers Without a Library Implementation")
 */
#ifdef CC45_FUTURE

/* sprite_procedural_rigging.h — Phase 42: Procedural Rigging
 *
 * Automatic skeleton generation and bone placement from sprite shapes.
 */

#ifndef SPRITE_PROCEDURAL_RIGGING_H
#define SPRITE_PROCEDURAL_RIGGING_H

#include <sprite_skeletal_animation.h>

typedef void *sprite_rig_generator_t;
typedef void *sprite_bone_weight_t;

#define INVALID_RIG_GENERATOR NULL
#define INVALID_BONE_WEIGHT NULL

typedef enum {
    RIG_BIPED = 0,
    RIG_QUADRUPED = 1,
    RIG_CUSTOM = 2,
} sprite_rig_type_t;

/* Rig Generation */
sprite_rig_generator_t sprite_rig_generator_create(sprite_rig_type_t type);
void sprite_rig_generator_destroy(sprite_rig_generator_t generator);
sprite_skeleton_t sprite_rig_generator_generate(sprite_rig_generator_t generator,
                                                int width, int height, int seed);

/* Bone Placement */
int sprite_rig_generator_set_bone_count(sprite_rig_generator_t generator, int count);
int sprite_rig_generator_optimize_placement(sprite_rig_generator_t generator);

/* Weight Mapping */
sprite_bone_weight_t sprite_bone_weight_create(sprite_skeleton_t skeleton);
void sprite_bone_weight_destroy(sprite_bone_weight_t weights);
void sprite_bone_weight_set_influence(sprite_bone_weight_t weights, int x, int y,
                                      sprite_bone_t bone, float weight);
float sprite_bone_weight_get_influence(sprite_bone_weight_t weights, int x, int y,
                                       sprite_bone_t bone);
int sprite_bone_weight_compute_deformation(sprite_bone_weight_t weights, int x, int y,
                                           float *out_x, float *out_y);

/* Template Rigging */
int sprite_rig_generator_load_template(sprite_rig_generator_t generator,
                                       const char *template_name);
int sprite_rig_generator_save_template(sprite_rig_generator_t generator,
                                       const char *template_name);

#endif

#endif /* CC45_FUTURE */
