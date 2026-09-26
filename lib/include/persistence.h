/* sprite_save_load.h — Save/Load Systems (Phases 71-75)
 *
 * Phases 71-75: Persistence, checkpoints, progression, serialization, recovery
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

typedef void *sprite_savedata_t;
typedef void *sprite_checkpoint_t;
typedef void *sprite_progression_t;
typedef void *sprite_serializer_t;
typedef void *sprite_recovery_t;

#define INVALID_SAVEDATA NULL
#define INVALID_CHECKPOINT NULL
#define INVALID_PROGRESSION NULL
#define INVALID_SERIALIZER NULL
#define INVALID_RECOVERY NULL

typedef enum {
    SAVE_SLOT_1 = 0,
    SAVE_SLOT_2 = 1,
    SAVE_SLOT_3 = 2,
    SAVE_AUTO = 3,
} sprite_save_slot_t;

typedef enum {
    RECOVERY_NONE = 0,
    RECOVERY_CHECKPOINT = 1,
    RECOVERY_AUTO_SAVE = 2,
    RECOVERY_BACKUP = 3,
} sprite_recovery_type_t;

/* Phase 71: File Persistence */
sprite_savedata_t sprite_savedata_create(const char *filename);
void sprite_savedata_destroy(sprite_savedata_t savedata);
int sprite_savedata_write_int(sprite_savedata_t savedata, const char *key, int value);
int sprite_savedata_read_int(sprite_savedata_t savedata, const char *key, int *value);
int sprite_savedata_write_float(sprite_savedata_t savedata, const char *key, float value);
int sprite_savedata_read_float(sprite_savedata_t savedata, const char *key, float *value);
int sprite_savedata_write_string(sprite_savedata_t savedata, const char *key, const char *value);
int sprite_savedata_read_string(sprite_savedata_t savedata, const char *key, char *value, int max_len);
int sprite_savedata_flush(sprite_savedata_t savedata);

/* Phase 72: Checkpoints */
sprite_checkpoint_t sprite_checkpoint_create(int level_id);
void sprite_checkpoint_destroy(sprite_checkpoint_t checkpoint);
int sprite_checkpoint_save(sprite_checkpoint_t checkpoint, sprite_savedata_t savedata);
int sprite_checkpoint_load(sprite_checkpoint_t checkpoint, sprite_savedata_t savedata);
void sprite_checkpoint_set_position(sprite_checkpoint_t checkpoint, float x, float y);
void sprite_checkpoint_set_health(sprite_checkpoint_t checkpoint, int health);
int sprite_checkpoint_restore(sprite_checkpoint_t checkpoint);

/* Phase 73: Progression Tracking */
sprite_progression_t sprite_progression_create(int max_levels);
void sprite_progression_destroy(sprite_progression_t progression);
int sprite_progression_mark_level_complete(sprite_progression_t progression, int level_id);
int sprite_progression_is_level_complete(sprite_progression_t progression, int level_id);
int sprite_progression_unlock_level(sprite_progression_t progression, int level_id);
int sprite_progression_is_level_unlocked(sprite_progression_t progression, int level_id);
int sprite_progression_get_completion_percentage(sprite_progression_t progression);

/* Phase 74: Serialization */
sprite_serializer_t sprite_serializer_create(void);
void sprite_serializer_destroy(sprite_serializer_t serializer);
int sprite_serializer_serialize_object(sprite_serializer_t serializer, void *object, int size);
int sprite_serializer_deserialize_object(sprite_serializer_t serializer, void *object, int max_size);
int sprite_serializer_get_buffer_size(sprite_serializer_t serializer);
void *sprite_serializer_get_buffer(sprite_serializer_t serializer);
int sprite_serializer_compress(sprite_serializer_t serializer);
int sprite_serializer_decompress(sprite_serializer_t serializer);

/* Phase 75: Recovery & Backup */
sprite_recovery_t sprite_recovery_create(int max_backups);
void sprite_recovery_destroy(sprite_recovery_t recovery);
int sprite_recovery_create_backup(sprite_recovery_t recovery, const char *savedata_path);
int sprite_recovery_list_backups(sprite_recovery_t recovery, char **backup_paths, int max_backups);
int sprite_recovery_restore_backup(sprite_recovery_t recovery, int backup_index);
int sprite_recovery_verify_integrity(sprite_recovery_t recovery, const char *savedata_path);
int sprite_recovery_get_last_error(sprite_recovery_t recovery);

#endif
