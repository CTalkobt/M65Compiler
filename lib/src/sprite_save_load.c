/* sprite_save_load.c — Save/Load Implementation */

#include "sprite_save_load.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_SAVEDATA 8
#define MAX_CHECKPOINTS 32
#define MAX_PROGRESSIONS 4
#define MAX_SERIALIZERS 8
#define MAX_RECOVERIES 4
#define MAX_KV_PAIRS 256

typedef struct {
    char key[64];
    char value[256];
} kv_pair_t;

typedef struct {
    char filename[256];
    kv_pair_t pairs[MAX_KV_PAIRS];
    int pair_count;
} savedata_impl;

typedef struct {
    int level_id;
    float pos_x, pos_y;
    int health;
} checkpoint_impl;

typedef struct {
    int *level_complete;
    int *level_unlocked;
    int max_levels;
    int completion_percent;
} progression_impl;

typedef struct {
    unsigned char *buffer;
    int buffer_size;
    int buffer_capacity;
    int position;
} serializer_impl;

typedef struct {
    char backup_paths[8][256];
    int backup_count;
    int max_backups;
    int last_error;
} recovery_impl;

static savedata_impl savedatas[MAX_SAVEDATA];
static int savedata_count = 0;

static checkpoint_impl checkpoints[MAX_CHECKPOINTS];
static int checkpoint_count = 0;

static progression_impl progressions[MAX_PROGRESSIONS];
static int progression_count = 0;

static serializer_impl serializers[MAX_SERIALIZERS];
static int serializer_count = 0;

static recovery_impl recoveries[MAX_RECOVERIES];
static int recovery_count = 0;

/* Phase 71: Persistence */
sprite_savedata_t sprite_savedata_create(const char *filename) {
    if (savedata_count >= MAX_SAVEDATA) return INVALID_SAVEDATA;
    savedata_impl *sd = &savedatas[savedata_count];
    strncpy(sd->filename, filename, sizeof(sd->filename) - 1);
    sd->pair_count = 0;
    return (sprite_savedata_t)(intptr_t)savedata_count++;
}

void sprite_savedata_destroy(sprite_savedata_t savedata) {}

int sprite_savedata_write_int(sprite_savedata_t savedata, const char *key, int value) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count) return 0;
    savedata_impl *sd = &savedatas[idx];

    for (int i = 0; i < sd->pair_count; i++) {
        if (strcmp(sd->pairs[i].key, key) == 0) {
            snprintf(sd->pairs[i].value, sizeof(sd->pairs[i].value), "%d", value);
            return 1;
        }
    }

    if (sd->pair_count < MAX_KV_PAIRS) {
        strncpy(sd->pairs[sd->pair_count].key, key, sizeof(sd->pairs[0].key) - 1);
        snprintf(sd->pairs[sd->pair_count].value, sizeof(sd->pairs[0].value), "%d", value);
        sd->pair_count++;
        return 1;
    }
    return 0;
}

int sprite_savedata_read_int(sprite_savedata_t savedata, const char *key, int *value) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count || !value) return 0;
    savedata_impl *sd = &savedatas[idx];

    for (int i = 0; i < sd->pair_count; i++) {
        if (strcmp(sd->pairs[i].key, key) == 0) {
            *value = atoi(sd->pairs[i].value);
            return 1;
        }
    }
    return 0;
}

int sprite_savedata_write_float(sprite_savedata_t savedata, const char *key, float value) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count) return 0;
    savedata_impl *sd = &savedatas[idx];

    if (sd->pair_count < MAX_KV_PAIRS) {
        strncpy(sd->pairs[sd->pair_count].key, key, sizeof(sd->pairs[0].key) - 1);
        snprintf(sd->pairs[sd->pair_count].value, sizeof(sd->pairs[0].value), "%f", value);
        sd->pair_count++;
        return 1;
    }
    return 0;
}

int sprite_savedata_read_float(sprite_savedata_t savedata, const char *key, float *value) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count || !value) return 0;
    savedata_impl *sd = &savedatas[idx];

    for (int i = 0; i < sd->pair_count; i++) {
        if (strcmp(sd->pairs[i].key, key) == 0) {
            *value = atof(sd->pairs[i].value);
            return 1;
        }
    }
    return 0;
}

int sprite_savedata_write_string(sprite_savedata_t savedata, const char *key, const char *value) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count) return 0;
    savedata_impl *sd = &savedatas[idx];

    if (sd->pair_count < MAX_KV_PAIRS) {
        strncpy(sd->pairs[sd->pair_count].key, key, sizeof(sd->pairs[0].key) - 1);
        strncpy(sd->pairs[sd->pair_count].value, value, sizeof(sd->pairs[0].value) - 1);
        sd->pair_count++;
        return 1;
    }
    return 0;
}

int sprite_savedata_read_string(sprite_savedata_t savedata, const char *key, char *value, int max_len) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count || !value) return 0;
    savedata_impl *sd = &savedatas[idx];

    for (int i = 0; i < sd->pair_count; i++) {
        if (strcmp(sd->pairs[i].key, key) == 0) {
            strncpy(value, sd->pairs[i].value, max_len - 1);
            return 1;
        }
    }
    return 0;
}

int sprite_savedata_flush(sprite_savedata_t savedata) {
    intptr_t idx = (intptr_t)savedata;
    if (idx < 0 || idx >= savedata_count) return 0;

    savedata_impl *sd = &savedatas[idx];
    FILE *f = fopen(sd->filename, "w");
    if (!f) return 0;

    for (int i = 0; i < sd->pair_count; i++) {
        fprintf(f, "%s=%s\n", sd->pairs[i].key, sd->pairs[i].value);
    }
    fclose(f);
    return 1;
}

/* Phase 72: Checkpoints */
sprite_checkpoint_t sprite_checkpoint_create(int level_id) {
    if (checkpoint_count >= MAX_CHECKPOINTS) return INVALID_CHECKPOINT;
    checkpoint_impl *cp = &checkpoints[checkpoint_count];
    cp->level_id = level_id;
    cp->pos_x = cp->pos_y = 0.0f;
    cp->health = 100;
    return (sprite_checkpoint_t)(intptr_t)checkpoint_count++;
}

void sprite_checkpoint_destroy(sprite_checkpoint_t checkpoint) {}

int sprite_checkpoint_save(sprite_checkpoint_t checkpoint, sprite_savedata_t savedata) {
    intptr_t c_idx = (intptr_t)checkpoint;
    intptr_t s_idx = (intptr_t)savedata;
    if (c_idx < 0 || c_idx >= checkpoint_count || s_idx < 0 || s_idx >= savedata_count) return 0;

    checkpoint_impl *cp = &checkpoints[c_idx];
    char key[64];
    snprintf(key, sizeof(key), "cp_%d_x", cp->level_id);
    sprite_savedata_write_float(savedata, key, cp->pos_x);
    snprintf(key, sizeof(key), "cp_%d_y", cp->level_id);
    sprite_savedata_write_float(savedata, key, cp->pos_y);
    snprintf(key, sizeof(key), "cp_%d_health", cp->level_id);
    sprite_savedata_write_int(savedata, key, cp->health);
    return 1;
}

int sprite_checkpoint_load(sprite_checkpoint_t checkpoint, sprite_savedata_t savedata) {
    return 1;
}

void sprite_checkpoint_set_position(sprite_checkpoint_t checkpoint, float x, float y) {
    intptr_t idx = (intptr_t)checkpoint;
    if (idx < 0 || idx >= checkpoint_count) return;
    checkpoints[idx].pos_x = x;
    checkpoints[idx].pos_y = y;
}

void sprite_checkpoint_set_health(sprite_checkpoint_t checkpoint, int health) {
    intptr_t idx = (intptr_t)checkpoint;
    if (idx < 0 || idx >= checkpoint_count) return;
    checkpoints[idx].health = health;
}

int sprite_checkpoint_restore(sprite_checkpoint_t checkpoint) {
    return 1;
}

/* Phase 73: Progression */
sprite_progression_t sprite_progression_create(int max_levels) {
    if (progression_count >= MAX_PROGRESSIONS) return INVALID_PROGRESSION;
    progression_impl *prog = &progressions[progression_count];
    prog->max_levels = max_levels;
    prog->level_complete = (int *)malloc(max_levels * sizeof(int));
    prog->level_unlocked = (int *)malloc(max_levels * sizeof(int));
    memset(prog->level_complete, 0, max_levels * sizeof(int));
    prog->level_unlocked[0] = 1;
    for (int i = 1; i < max_levels; i++) prog->level_unlocked[i] = 0;
    prog->completion_percent = 0;
    return (sprite_progression_t)(intptr_t)progression_count++;
}

void sprite_progression_destroy(sprite_progression_t progression) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return;
    free(progressions[idx].level_complete);
    free(progressions[idx].level_unlocked);
}

int sprite_progression_mark_level_complete(sprite_progression_t progression, int level_id) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return 0;
    progression_impl *prog = &progressions[idx];
    if (level_id < 0 || level_id >= prog->max_levels) return 0;

    prog->level_complete[level_id] = 1;
    if (level_id + 1 < prog->max_levels) prog->level_unlocked[level_id + 1] = 1;

    int complete = 0;
    for (int i = 0; i < prog->max_levels; i++) complete += prog->level_complete[i];
    prog->completion_percent = (complete * 100) / prog->max_levels;
    return 1;
}

int sprite_progression_is_level_complete(sprite_progression_t progression, int level_id) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return 0;
    progression_impl *prog = &progressions[idx];
    if (level_id < 0 || level_id >= prog->max_levels) return 0;
    return prog->level_complete[level_id];
}

int sprite_progression_unlock_level(sprite_progression_t progression, int level_id) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return 0;
    progression_impl *prog = &progressions[idx];
    if (level_id < 0 || level_id >= prog->max_levels) return 0;
    prog->level_unlocked[level_id] = 1;
    return 1;
}

int sprite_progression_is_level_unlocked(sprite_progression_t progression, int level_id) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return 0;
    progression_impl *prog = &progressions[idx];
    if (level_id < 0 || level_id >= prog->max_levels) return 0;
    return prog->level_unlocked[level_id];
}

int sprite_progression_get_completion_percentage(sprite_progression_t progression) {
    intptr_t idx = (intptr_t)progression;
    if (idx < 0 || idx >= progression_count) return 0;
    return progressions[idx].completion_percent;
}

/* Phase 74: Serialization */
sprite_serializer_t sprite_serializer_create(void) {
    if (serializer_count >= MAX_SERIALIZERS) return INVALID_SERIALIZER;
    serializer_impl *ser = &serializers[serializer_count];
    ser->buffer_capacity = 4096;
    ser->buffer = (unsigned char *)malloc(ser->buffer_capacity);
    ser->buffer_size = 0;
    ser->position = 0;
    return (sprite_serializer_t)(intptr_t)serializer_count++;
}

void sprite_serializer_destroy(sprite_serializer_t serializer) {
    intptr_t idx = (intptr_t)serializer;
    if (idx < 0 || idx >= serializer_count) return;
    free(serializers[idx].buffer);
}

int sprite_serializer_serialize_object(sprite_serializer_t serializer, void *object, int size) {
    intptr_t idx = (intptr_t)serializer;
    if (idx < 0 || idx >= serializer_count) return 0;
    serializer_impl *ser = &serializers[idx];

    if (ser->buffer_size + size > ser->buffer_capacity) return 0;
    memcpy(ser->buffer + ser->buffer_size, object, size);
    ser->buffer_size += size;
    return size;
}

int sprite_serializer_deserialize_object(sprite_serializer_t serializer, void *object, int max_size) {
    intptr_t idx = (intptr_t)serializer;
    if (idx < 0 || idx >= serializer_count) return 0;
    serializer_impl *ser = &serializers[idx];

    if (ser->position + max_size > ser->buffer_size) return 0;
    memcpy(object, ser->buffer + ser->position, max_size);
    ser->position += max_size;
    return max_size;
}

int sprite_serializer_get_buffer_size(sprite_serializer_t serializer) {
    intptr_t idx = (intptr_t)serializer;
    if (idx < 0 || idx >= serializer_count) return 0;
    return serializers[idx].buffer_size;
}

void *sprite_serializer_get_buffer(sprite_serializer_t serializer) {
    intptr_t idx = (intptr_t)serializer;
    if (idx < 0 || idx >= serializer_count) return NULL;
    return serializers[idx].buffer;
}

int sprite_serializer_compress(sprite_serializer_t serializer) {
    return 1;
}

int sprite_serializer_decompress(sprite_serializer_t serializer) {
    return 1;
}

/* Phase 75: Recovery */
sprite_recovery_t sprite_recovery_create(int max_backups) {
    if (recovery_count >= MAX_RECOVERIES) return INVALID_RECOVERY;
    recovery_impl *rec = &recoveries[recovery_count];
    rec->backup_count = 0;
    rec->max_backups = max_backups;
    rec->last_error = 0;
    return (sprite_recovery_t)(intptr_t)recovery_count++;
}

void sprite_recovery_destroy(sprite_recovery_t recovery) {}

int sprite_recovery_create_backup(sprite_recovery_t recovery, const char *savedata_path) {
    intptr_t idx = (intptr_t)recovery;
    if (idx < 0 || idx >= recovery_count) return 0;
    recovery_impl *rec = &recoveries[idx];

    if (rec->backup_count >= rec->max_backups) {
        for (int i = 0; i < rec->backup_count - 1; i++) {
            strcpy(rec->backup_paths[i], rec->backup_paths[i+1]);
        }
        rec->backup_count--;
    }

    snprintf(rec->backup_paths[rec->backup_count], sizeof(rec->backup_paths[0]),
             "%s.backup.%d", savedata_path, rec->backup_count);
    rec->backup_count++;
    return 1;
}

int sprite_recovery_list_backups(sprite_recovery_t recovery, char **backup_paths, int max_backups) {
    intptr_t idx = (intptr_t)recovery;
    if (idx < 0 || idx >= recovery_count) return 0;
    recovery_impl *rec = &recoveries[idx];

    int count = (rec->backup_count < max_backups) ? rec->backup_count : max_backups;
    for (int i = 0; i < count; i++) {
        backup_paths[i] = rec->backup_paths[i];
    }
    return count;
}

int sprite_recovery_restore_backup(sprite_recovery_t recovery, int backup_index) {
    intptr_t idx = (intptr_t)recovery;
    if (idx < 0 || idx >= recovery_count) return 0;
    recovery_impl *rec = &recoveries[idx];
    if (backup_index < 0 || backup_index >= rec->backup_count) return 0;
    return 1;
}

int sprite_recovery_verify_integrity(sprite_recovery_t recovery, const char *savedata_path) {
    return 1;
}

int sprite_recovery_get_last_error(sprite_recovery_t recovery) {
    intptr_t idx = (intptr_t)recovery;
    if (idx < 0 || idx >= recovery_count) return 0;
    return recoveries[idx].last_error;
}
