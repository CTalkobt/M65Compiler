/* sprite_motion_matching.c — Motion Matching Implementation */

#include "sprite_motion_matching.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_MOTION_DBS 4
#define MAX_MOTIONS 64
#define MAX_QUERIES 16
#define MAX_FEATURES 32

typedef struct {
    char name[64];
    sprite_anim_tree_t animation;
} motion_entry_t;

typedef struct {
    motion_entry_t motions[MAX_MOTIONS];
    int motion_count;
    int capacity;
} motion_database_impl;

typedef struct {
    motion_database_impl *db;
    sprite_motion_features_t query_features;
    char best_match[64];
    float best_cost;
    float match_costs[MAX_MOTIONS];
} motion_query_impl;

typedef struct {
    sprite_motion_feature_type_t type;
    float weight;
} motion_feature_impl;

static motion_database_impl motion_dbs[MAX_MOTION_DBS];
static int motion_db_count = 0;

static motion_query_impl motion_queries[MAX_QUERIES];
static int motion_query_count = 0;

static motion_feature_impl motion_features[MAX_FEATURES];
static int motion_feature_count = 0;

sprite_motion_database_t sprite_motion_database_create(int capacity) {
    if (motion_db_count >= MAX_MOTION_DBS) return INVALID_MOTION_DB;
    motion_database_impl *db = &motion_dbs[motion_db_count];
    db->motion_count = 0;
    db->capacity = (capacity > MAX_MOTIONS) ? MAX_MOTIONS : capacity;
    return (sprite_motion_database_t)(intptr_t)motion_db_count++;
}

void sprite_motion_database_destroy(sprite_motion_database_t db) {}

int sprite_motion_database_add_motion(sprite_motion_database_t db,
                                       const char *motion_name,
                                       sprite_anim_tree_t animation) {
    intptr_t idx = (intptr_t)db;
    if (idx < 0 || idx >= motion_db_count) return 0;

    motion_database_impl *database = &motion_dbs[idx];
    if (database->motion_count >= database->capacity) return 0;

    motion_entry_t *entry = &database->motions[database->motion_count];
    strncpy(entry->name, motion_name, sizeof(entry->name) - 1);
    entry->animation = animation;

    return ++database->motion_count;
}

int sprite_motion_database_get_motion_count(sprite_motion_database_t db) {
    intptr_t idx = (intptr_t)db;
    if (idx < 0 || idx >= motion_db_count) return 0;
    return motion_dbs[idx].motion_count;
}

sprite_motion_query_t sprite_motion_query_create(sprite_motion_database_t db) {
    if (motion_query_count >= MAX_QUERIES) return INVALID_QUERY;

    motion_query_impl *query = &motion_queries[motion_query_count];
    intptr_t idx = (intptr_t)db;
    query->db = (idx >= 0 && idx < motion_db_count) ? &motion_dbs[idx] : NULL;
    memset(&query->query_features, 0, sizeof(query->query_features));
    strcpy(query->best_match, "");
    query->best_cost = 1e9f;

    return (sprite_motion_query_t)(intptr_t)motion_query_count++;
}

void sprite_motion_query_destroy(sprite_motion_query_t query) {}

void sprite_motion_query_set_features(sprite_motion_query_t query,
                                       sprite_motion_features_t features) {
    intptr_t idx = (intptr_t)query;
    if (idx < 0 || idx >= motion_query_count) return;
    motion_queries[idx].query_features = features;
}

static float compute_motion_cost(motion_query_impl *query, motion_entry_t *motion) {
    float cost = 0.0f;

    float vel_dist = sqrtf(query->query_features.velocity_x * query->query_features.velocity_x +
                          query->query_features.velocity_y * query->query_features.velocity_y);
    cost += fabsf(vel_dist - 0.5f) * 10.0f;

    cost += fabsf(query->query_features.phase - 0.5f) * 5.0f;

    return cost;
}

const char *sprite_motion_query_find_best_match(sprite_motion_query_t query) {
    intptr_t idx = (intptr_t)query;
    if (idx < 0 || idx >= motion_query_count) return "";

    motion_query_impl *q = &motion_queries[idx];
    if (!q->db || q->db->motion_count == 0) return "";

    q->best_cost = 1e9f;
    strcpy(q->best_match, "");

    for (int i = 0; i < q->db->motion_count; i++) {
        float cost = compute_motion_cost(q, &q->db->motions[i]);
        q->match_costs[i] = cost;

        if (cost < q->best_cost) {
            q->best_cost = cost;
            strncpy(q->best_match, q->db->motions[i].name, sizeof(q->best_match) - 1);
        }
    }

    return q->best_match;
}

float sprite_motion_query_get_match_cost(sprite_motion_query_t query) {
    intptr_t idx = (intptr_t)query;
    if (idx < 0 || idx >= motion_query_count) return 1e9f;
    return motion_queries[idx].best_cost;
}

int sprite_motion_query_get_top_matches(sprite_motion_query_t query,
                                         const char **matches, int max_count) {
    intptr_t idx = (intptr_t)query;
    if (idx < 0 || idx >= motion_query_count) return 0;

    motion_query_impl *q = &motion_queries[idx];
    if (!q->db) return 0;

    int count = 0;
    for (int i = 0; i < q->db->motion_count && count < max_count; i++) {
        matches[count++] = q->db->motions[i].name;
    }
    return count;
}

sprite_motion_feature_t sprite_motion_feature_create(sprite_motion_feature_type_t type) {
    if (motion_feature_count >= MAX_FEATURES) return INVALID_FEATURE;
    motion_feature_impl *feat = &motion_features[motion_feature_count];
    feat->type = type;
    feat->weight = 1.0f;
    return (sprite_motion_feature_t)(intptr_t)motion_feature_count++;
}

void sprite_motion_feature_destroy(sprite_motion_feature_t feature) {}

void sprite_motion_feature_set_weight(sprite_motion_feature_t feature, float weight) {
    intptr_t idx = (intptr_t)feature;
    if (idx < 0 || idx >= motion_feature_count) return;
    motion_features[idx].weight = weight;
}

float sprite_motion_feature_compute_distance(sprite_motion_feature_t feature,
                                             float value1, float value2) {
    intptr_t idx = (intptr_t)feature;
    if (idx < 0 || idx >= motion_feature_count) return 0.0f;

    motion_feature_impl *feat = &motion_features[idx];
    float dist = fabsf(value1 - value2);
    return dist * feat->weight;
}

int sprite_motion_matching_blend(sprite_motion_query_t query,
                                  const char *current_motion,
                                  const char *target_motion,
                                  float blend_time) {
    return 1;
}

int sprite_motion_matching_update(sprite_motion_query_t query, int delta_ms) {
    intptr_t idx = (intptr_t)query;
    if (idx < 0 || idx >= motion_query_count) return 0;
    return 1;
}
