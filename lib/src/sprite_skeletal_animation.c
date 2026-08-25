/* sprite_skeletal_animation.c — Skeletal Animation Implementation */

#include "sprite_skeletal_animation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define MAX_SKELETONS    32
#define MAX_BONES_PER_SKELETON 64
#define MAX_TRACKS_PER_SKELETON 128
#define MAX_KEYFRAMES_PER_TRACK 256
#define MAX_IK_CONSTRAINTS 16

typedef struct {
    char name[64];
    sprite_bone_t parent;
    float x, y;
    float rotation;
    float scale_x, scale_y;
    sprite_bone_t children[32];
    int child_count;
    int index;
} sprite_bone_t_impl;

typedef struct {
    float time;
    float value[3];
} sprite_keyframe_t_impl;

typedef struct {
    sprite_bone_t bone;
    sprite_track_type_t type;
    sprite_keyframe_t_impl keyframes[MAX_KEYFRAMES_PER_TRACK];
    int keyframe_count;
    int index;
} sprite_track_t_impl;

typedef struct {
    sprite_bone_t root;
    int target;
    int chain_length;
    float tolerance;
} sprite_ik_constraint_t;

typedef struct {
    sprite_bone_t_impl bones[MAX_BONES_PER_SKELETON];
    int bone_count;
    sprite_track_t_impl tracks[MAX_TRACKS_PER_SKELETON];
    int track_count;
    sprite_ik_constraint_t ik_constraints[MAX_IK_CONSTRAINTS];
    int ik_count;
    int is_playing;
    int current_time;
    float playback_speed;
} sprite_skeleton_impl;

static sprite_skeleton_impl skeletons[MAX_SKELETONS];
static int skeleton_count = 0;

sprite_skeleton_t sprite_skeleton_create(void) {
    if (skeleton_count >= MAX_SKELETONS) return INVALID_SKELETON;

    sprite_skeleton_impl *skel = &skeletons[skeleton_count];
    memset(skel, 0, sizeof(*skel));
    skel->playback_speed = 1.0f;

    return (sprite_skeleton_t)(intptr_t)skeleton_count++;
}

void sprite_skeleton_destroy(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
}

int sprite_skeleton_get_info(sprite_skeleton_t skeleton, sprite_skeleton_info_t *info) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count || !info) return 0;

    sprite_skeleton_impl *skel = &skeletons[idx];
    info->skeleton = skeleton;
    info->bone_count = skel->bone_count;
    info->track_count = skel->track_count;
    info->current_time = skel->current_time;
    info->is_playing = skel->is_playing;
    return 1;
}

int sprite_skeleton_update(sprite_skeleton_t skeleton, int delta_ms) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (!skel->is_playing) return 0;

    skel->current_time += (int)(delta_ms * skel->playback_speed);

    for (int t = 0; t < skel->track_count; t++) {
        sprite_track_t_impl *track = &skel->tracks[t];

        int kf_idx = 0;
        for (int k = 0; k < track->keyframe_count - 1; k++) {
            if (track->keyframes[k].time <= skel->current_time &&
                skel->current_time < track->keyframes[k+1].time) {
                kf_idx = k;
                break;
            }
        }

        sprite_keyframe_t_impl *kf1 = &track->keyframes[kf_idx];
        sprite_keyframe_t_impl *kf2 = (kf_idx + 1 < track->keyframe_count)
            ? &track->keyframes[kf_idx + 1]
            : kf1;

        float t_local = 0.0f;
        if (kf2->time > kf1->time) {
            t_local = (float)(skel->current_time - kf1->time) /
                      (float)(kf2->time - kf1->time);
            if (t_local > 1.0f) t_local = 1.0f;
        }

        sprite_bone_t_impl *bone = (sprite_bone_t_impl *)track->bone;

        for (int i = 0; i < 3; i++) {
            float val = kf1->value[i] * (1.0f - t_local) +
                       kf2->value[i] * t_local;

            if (track->type == TRACK_POSITION) {
                if (i == 0) bone->x = val;
                else if (i == 1) bone->y = val;
            } else if (track->type == TRACK_ROTATION) {
                bone->rotation = val;
            } else if (track->type == TRACK_SCALE) {
                if (i == 0) bone->scale_x = val;
                else if (i == 1) bone->scale_y = val;
            }
        }
    }

    return skel->track_count;
}

sprite_bone_t sprite_skeleton_add_bone(sprite_skeleton_t skeleton,
                                        const char *name,
                                        sprite_bone_t parent) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return INVALID_BONE;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (skel->bone_count >= MAX_BONES_PER_SKELETON) return INVALID_BONE;

    sprite_bone_t_impl *bone = &skel->bones[skel->bone_count];
    strncpy(bone->name, name, sizeof(bone->name) - 1);
    bone->parent = parent;
    bone->x = bone->y = 0.0f;
    bone->rotation = 0.0f;
    bone->scale_x = bone->scale_y = 1.0f;
    bone->child_count = 0;
    bone->index = skel->bone_count;

    if (parent != INVALID_BONE) {
        sprite_bone_t_impl *p = (sprite_bone_t_impl *)parent;
        if (p->child_count < 32) {
            p->children[p->child_count++] = (sprite_bone_t)bone;
        }
    }

    skel->bone_count++;
    return (sprite_bone_t)bone;
}

int sprite_skeleton_remove_bone(sprite_skeleton_t skeleton, sprite_bone_t bone) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;

    sprite_skeleton_impl *skel = &skeletons[idx];
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;

    for (int i = 0; i < skel->bone_count; i++) {
        if (&skel->bones[i] == b) {
            if (b->parent != INVALID_BONE) {
                sprite_bone_t_impl *p = (sprite_bone_t_impl *)b->parent;
                for (int c = 0; c < p->child_count; c++) {
                    if (p->children[c] == bone) {
                        p->children[c] = p->children[p->child_count-1];
                        p->child_count--;
                        break;
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

sprite_bone_t sprite_skeleton_find_bone(sprite_skeleton_t skeleton,
                                         const char *name) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return INVALID_BONE;

    sprite_skeleton_impl *skel = &skeletons[idx];
    for (int i = 0; i < skel->bone_count; i++) {
        if (strcmp(skel->bones[i].name, name) == 0) {
            return (sprite_bone_t)&skel->bones[i];
        }
    }
    return INVALID_BONE;
}

int sprite_skeleton_get_bone_count(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;
    return skeletons[idx].bone_count;
}

sprite_bone_t sprite_skeleton_get_bone(sprite_skeleton_t skeleton, int index) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return INVALID_BONE;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (index < 0 || index >= skel->bone_count) return INVALID_BONE;
    return (sprite_bone_t)&skel->bones[index];
}

void sprite_skeleton_set_bone_position(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float x, float y) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (b) {
        b->x = x;
        b->y = y;
    }
}

void sprite_skeleton_get_bone_position(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float *x, float *y) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (b && x && y) {
        *x = b->x;
        *y = b->y;
    }
}

void sprite_skeleton_set_bone_rotation(sprite_skeleton_t skeleton,
                                        sprite_bone_t bone,
                                        float rotation) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (b) b->rotation = rotation;
}

float sprite_skeleton_get_bone_rotation(sprite_skeleton_t skeleton,
                                         sprite_bone_t bone) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    return b ? b->rotation : 0.0f;
}

void sprite_skeleton_set_bone_scale(sprite_skeleton_t skeleton,
                                     sprite_bone_t bone,
                                     float scale_x, float scale_y) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (b) {
        b->scale_x = scale_x;
        b->scale_y = scale_y;
    }
}

void sprite_skeleton_get_bone_scale(sprite_skeleton_t skeleton,
                                     sprite_bone_t bone,
                                     float *scale_x, float *scale_y) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (b && scale_x && scale_y) {
        *scale_x = b->scale_x;
        *scale_y = b->scale_y;
    }
}

sprite_track_t sprite_skeleton_add_track(sprite_skeleton_t skeleton,
                                          sprite_bone_t bone,
                                          sprite_track_type_t type) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return INVALID_TRACK;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (skel->track_count >= MAX_TRACKS_PER_SKELETON) return INVALID_TRACK;

    sprite_track_t_impl *track = &skel->tracks[skel->track_count];
    track->bone = bone;
    track->type = type;
    track->keyframe_count = 0;
    track->index = skel->track_count;

    skel->track_count++;
    return (sprite_track_t)track;
}

int sprite_skeleton_remove_track(sprite_skeleton_t skeleton, sprite_track_t track) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;

    sprite_skeleton_impl *skel = &skeletons[idx];
    sprite_track_t_impl *t = (sprite_track_t_impl *)track;

    for (int i = 0; i < skel->track_count; i++) {
        if (&skel->tracks[i] == t) {
            return 1;
        }
    }
    return 0;
}

sprite_keyframe_t sprite_skeleton_add_keyframe(sprite_skeleton_t skeleton,
                                                sprite_track_t track,
                                                float time,
                                                float value1,
                                                float value2,
                                                float value3) {
    sprite_track_t_impl *t = (sprite_track_t_impl *)track;
    if (!t || t->keyframe_count >= MAX_KEYFRAMES_PER_TRACK) return INVALID_KEYFRAME;

    sprite_keyframe_t_impl *kf = &t->keyframes[t->keyframe_count];
    kf->time = (int)time;
    kf->value[0] = value1;
    kf->value[1] = value2;
    kf->value[2] = value3;

    t->keyframe_count++;
    return (sprite_keyframe_t)kf;
}

int sprite_skeleton_remove_keyframe(sprite_skeleton_t skeleton,
                                     sprite_track_t track,
                                     sprite_keyframe_t keyframe) {
    return 1;
}

int sprite_skeleton_get_track_count(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;
    return skeletons[idx].track_count;
}

sprite_track_t sprite_skeleton_get_track(sprite_skeleton_t skeleton, int index) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return INVALID_TRACK;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (index < 0 || index >= skel->track_count) return INVALID_TRACK;
    return (sprite_track_t)&skel->tracks[index];
}

int sprite_skeleton_get_keyframe_count(sprite_skeleton_t skeleton,
                                        sprite_track_t track) {
    sprite_track_t_impl *t = (sprite_track_t_impl *)track;
    return t ? t->keyframe_count : 0;
}

void sprite_skeleton_play(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
    skeletons[idx].is_playing = 1;
}

void sprite_skeleton_pause(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
    skeletons[idx].is_playing = 0;
}

void sprite_skeleton_stop(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
    skeletons[idx].is_playing = 0;
    skeletons[idx].current_time = 0;
}

int sprite_skeleton_is_playing(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;
    return skeletons[idx].is_playing;
}

int sprite_skeleton_get_animation_time(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;
    return skeletons[idx].current_time;
}

void sprite_skeleton_set_animation_time(sprite_skeleton_t skeleton, int time_ms) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
    skeletons[idx].current_time = time_ms;
}

float sprite_skeleton_get_animation_duration(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0.0f;

    sprite_skeleton_impl *skel = &skeletons[idx];
    float max_time = 0.0f;

    for (int i = 0; i < skel->track_count; i++) {
        if (skel->tracks[i].keyframe_count > 0) {
            float t = skel->tracks[i].keyframes[skel->tracks[i].keyframe_count - 1].time;
            if (t > max_time) max_time = t;
        }
    }
    return max_time;
}

void sprite_skeleton_set_playback_speed(sprite_skeleton_t skeleton, float speed) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;
    if (speed < 0.1f) speed = 0.1f;
    if (speed > 10.0f) speed = 10.0f;
    skeletons[idx].playback_speed = speed;
}

int sprite_skeleton_add_ik_constraint(sprite_skeleton_t skeleton,
                                       sprite_bone_t chain_root,
                                       sprite_bone_t chain_tip,
                                       sprite_bone_t target,
                                       int chain_length,
                                       float tolerance) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return 0;

    sprite_skeleton_impl *skel = &skeletons[idx];
    if (skel->ik_count >= MAX_IK_CONSTRAINTS) return 0;

    sprite_ik_constraint_t *constraint = &skel->ik_constraints[skel->ik_count];
    constraint->root = chain_root;
    constraint->target = (intptr_t)chain_tip;
    constraint->chain_length = chain_length;
    constraint->tolerance = tolerance;

    skel->ik_count++;
    return 1;
}

int sprite_skeleton_solve_ik(sprite_skeleton_t skeleton,
                              sprite_bone_t target) {
    return 1;
}

void sprite_skeleton_print_state(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;

    sprite_skeleton_impl *skel = &skeletons[idx];
    printf("Skeleton %ld: %d bones, %d tracks, time=%d, playing=%d\n",
           idx, skel->bone_count, skel->track_count, skel->current_time,
           skel->is_playing);
}

void sprite_skeleton_print_hierarchy(sprite_skeleton_t skeleton) {
    intptr_t idx = (intptr_t)skeleton;
    if (idx < 0 || idx >= skeleton_count) return;

    sprite_skeleton_impl *skel = &skeletons[idx];
    for (int i = 0; i < skel->bone_count; i++) {
        printf("  Bone %d: %s\n", i, skel->bones[i].name);
    }
}

void sprite_skeleton_print_bone(sprite_skeleton_t skeleton, sprite_bone_t bone) {
    sprite_bone_t_impl *b = (sprite_bone_t_impl *)bone;
    if (!b) return;

    printf("Bone: %s (pos=%.1f,%.1f rot=%.1f scale=%.1f,%.1f)\n",
           b->name, b->x, b->y, b->rotation, b->scale_x, b->scale_y);
}
