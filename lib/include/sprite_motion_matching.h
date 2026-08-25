/* sprite_motion_matching.h — Phase 43: Motion Matching
 *
 * Advanced animation blending via motion database queries.
 */

#ifndef SPRITE_MOTION_MATCHING_H
#define SPRITE_MOTION_MATCHING_H

#include <sprite_blend_spaces.h>

typedef void *sprite_motion_database_t;
typedef void *sprite_motion_query_t;
typedef void *sprite_motion_feature_t;

#define INVALID_MOTION_DB NULL
#define INVALID_QUERY NULL
#define INVALID_FEATURE NULL

typedef struct {
    float phase;
    float velocity_x, velocity_y;
    float acceleration_x, acceleration_y;
    float foot_position_x, foot_position_y;
    float body_rotation;
} sprite_motion_features_t;

typedef enum {
    MOTION_FEATURE_PHASE = 0,
    MOTION_FEATURE_VELOCITY = 1,
    MOTION_FEATURE_ACCELERATION = 2,
    MOTION_FEATURE_FOOT_POS = 3,
    MOTION_FEATURE_ROTATION = 4,
} sprite_motion_feature_type_t;

/* Motion Database */
sprite_motion_database_t sprite_motion_database_create(int capacity);
void sprite_motion_database_destroy(sprite_motion_database_t db);
int sprite_motion_database_add_motion(sprite_motion_database_t db,
                                       const char *motion_name,
                                       sprite_anim_tree_t animation);
int sprite_motion_database_get_motion_count(sprite_motion_database_t db);

/* Motion Query */
sprite_motion_query_t sprite_motion_query_create(sprite_motion_database_t db);
void sprite_motion_query_destroy(sprite_motion_query_t query);
void sprite_motion_query_set_features(sprite_motion_query_t query,
                                       sprite_motion_features_t features);
const char *sprite_motion_query_find_best_match(sprite_motion_query_t query);
float sprite_motion_query_get_match_cost(sprite_motion_query_t query);
int sprite_motion_query_get_top_matches(sprite_motion_query_t query,
                                         const char **matches, int max_count);

/* Feature Management */
sprite_motion_feature_t sprite_motion_feature_create(sprite_motion_feature_type_t type);
void sprite_motion_feature_destroy(sprite_motion_feature_t feature);
void sprite_motion_feature_set_weight(sprite_motion_feature_t feature, float weight);
float sprite_motion_feature_compute_distance(sprite_motion_feature_t feature,
                                             float value1, float value2);

/* Blending */
int sprite_motion_matching_blend(sprite_motion_query_t query,
                                  const char *current_motion,
                                  const char *target_motion,
                                  float blend_time);
int sprite_motion_matching_update(sprite_motion_query_t query, int delta_ms);

#endif
