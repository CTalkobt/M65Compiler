/* sprite_skeletal_animation.h — Skeletal Animation for MEGA65
 *
 * Provides skeletal animation with bone hierarchies and keyframe interpolation.
 *
 * Features:
 * - Bone hierarchies with parent-child relationships
 * - Keyframe animation with linear interpolation
 * - Transform tracks (position, rotation, scale)
 * - IK (Inverse Kinematics) constraints
 * - Animation blending across skeleton
 *
 * Usage:
 *   #include <sprite_skeletal_animation.h>
 *   sprite_skeleton_t skeleton = sprite_skeleton_create(root_sprite);
 *   sprite_bone_t root = sprite_skeleton_add_bone(skeleton, "root", NULL);
 *   sprite_bone_t left_arm = sprite_skeleton_add_bone(skeleton, "left_arm", root);
 *   sprite_skeleton_set_bone_position(skeleton, left_arm, 10.0f, -5.0f);
 */

#ifndef SPRITE_SKELETAL_ANIMATION_H
#define SPRITE_SKELETAL_ANIMATION_H

#include <sprite_animation_trees.h>

typedef void *sprite_skeleton_t;    /* Skeleton handle */
typedef void *sprite_bone_t;        /* Bone/joint handle */
typedef void *sprite_keyframe_t;    /* Keyframe handle */
typedef void *sprite_track_t;       /* Animation track handle */

#define INVALID_SKELETON    NULL
#define INVALID_BONE        NULL
#define INVALID_KEYFRAME    NULL
#define INVALID_TRACK       NULL

typedef enum {
    TRACK_POSITION = 0,
    TRACK_ROTATION = 1,
    TRACK_SCALE = 2,
} sprite_track_type_t;

typedef struct {
    sprite_bone_t bone;
    float x, y;
    float rotation;
    float scale_x, scale_y;
} sprite_bone_transform_t;

typedef struct {
    sprite_skeleton_t skeleton;
    int bone_count;
    int track_count;
    float current_time;
    int is_playing;
} sprite_skeleton_info_t;

/* ============================================================================
 * SKELETON CREATION & MANAGEMENT
 * ========================================================================== */

sprite_skeleton_t sprite_skeleton_create(void);
void sprite_skeleton_destroy(sprite_skeleton_t skeleton);
int sprite_skeleton_get_info(sprite_skeleton_t skeleton, sprite_skeleton_info_t *info);
int sprite_skeleton_update(sprite_skeleton_t skeleton, int delta_ms);

/* ============================================================================
 * BONE MANAGEMENT
 * ========================================================================== */

sprite_bone_t sprite_skeleton_add_bone(sprite_skeleton_t skeleton,
                                        const char *name,
                                        sprite_bone_t parent);
int sprite_skeleton_remove_bone(sprite_skeleton_t skeleton, sprite_bone_t bone);
sprite_bone_t sprite_skeleton_find_bone(sprite_skeleton_t skeleton,
                                         const char *name);
int sprite_skeleton_get_bone_count(sprite_skeleton_t skeleton);
sprite_bone_t sprite_skeleton_get_bone(sprite_skeleton_t skeleton, int index);

/* ============================================================================
 * BONE TRANSFORMS
 * ========================================================================== */

void sprite_skeleton_set_bone_position(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float x, float y);
void sprite_skeleton_get_bone_position(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float *x, float *y);
void sprite_skeleton_set_bone_rotation(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float rotation);
float sprite_skeleton_get_bone_rotation(sprite_skeleton_t skeleton,
                                         sprite_bone_t bone);
void sprite_skeleton_set_bone_scale(sprite_skeleton_t skeleton,
                                     sprite_bone_t bone,
                                     float scale_x, float scale_y);
void sprite_skeleton_get_bone_scale(sprite_skeleton_t skeleton,
                                     sprite_bone_t bone,
                                     float *scale_x, float *scale_y);

/* ============================================================================
 * KEYFRAME ANIMATION
 * ========================================================================== */

sprite_track_t sprite_skeleton_add_track(sprite_skeleton_t skeleton,
                                          sprite_bone_t bone,
                                          sprite_track_type_t type);
int sprite_skeleton_remove_track(sprite_skeleton_t skeleton, sprite_track_t track);
sprite_keyframe_t sprite_skeleton_add_keyframe(sprite_skeleton_t skeleton,
                                                sprite_track_t track,
                                                float time,
                                                float value1,
                                                float value2,
                                                float value3);
int sprite_skeleton_remove_keyframe(sprite_skeleton_t skeleton,
                                     sprite_track_t track,
                                     sprite_keyframe_t keyframe);
int sprite_skeleton_get_track_count(sprite_skeleton_t skeleton);
sprite_track_t sprite_skeleton_get_track(sprite_skeleton_t skeleton, int index);
int sprite_skeleton_get_keyframe_count(sprite_skeleton_t skeleton,
                                        sprite_track_t track);

/* ============================================================================
 * ANIMATION PLAYBACK
 * ========================================================================== */

void sprite_skeleton_play(sprite_skeleton_t skeleton);
void sprite_skeleton_pause(sprite_skeleton_t skeleton);
void sprite_skeleton_stop(sprite_skeleton_t skeleton);
int sprite_skeleton_is_playing(sprite_skeleton_t skeleton);
int sprite_skeleton_get_animation_time(sprite_skeleton_t skeleton);
void sprite_skeleton_set_animation_time(sprite_skeleton_t skeleton, int time_ms);
float sprite_skeleton_get_animation_duration(sprite_skeleton_t skeleton);
void sprite_skeleton_set_playback_speed(sprite_skeleton_t skeleton, float speed);

/* ============================================================================
 * IK CONSTRAINTS
 * ========================================================================== */

int sprite_skeleton_add_ik_constraint(sprite_skeleton_t skeleton,
                                       sprite_bone_t chain_root,
                                       sprite_bone_t chain_tip,
                                       sprite_bone_t target,
                                       int chain_length,
                                       float tolerance);
int sprite_skeleton_solve_ik(sprite_skeleton_t skeleton,
                              sprite_bone_t target);

/* ============================================================================
 * INSPECTION
 * ========================================================================== */

void sprite_skeleton_print_state(sprite_skeleton_t skeleton);
void sprite_skeleton_print_hierarchy(sprite_skeleton_t skeleton);
void sprite_skeleton_print_bone(sprite_skeleton_t skeleton, sprite_bone_t bone);

#endif
