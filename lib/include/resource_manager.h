/* sprite_resource_manager.h — Phase 62: Resource Manager
 *
 * Asset loading, caching, and memory pooling.
 */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

typedef void *sprite_resource_manager_t;
typedef void *sprite_resource_t;
typedef void *sprite_memory_pool_t;

#define INVALID_MANAGER NULL
#define INVALID_RESOURCE NULL
#define INVALID_POOL NULL

typedef enum {
    RESOURCE_SPRITE = 0,
    RESOURCE_ANIMATION = 1,
    RESOURCE_AUDIO = 2,
    RESOURCE_TEXTURE = 3,
    RESOURCE_DATA = 4,
} sprite_resource_type_t;

typedef struct {
    const char *path;
    sprite_resource_type_t type;
    int ref_count;
    void *data;
    int size;
} sprite_resource_info_t;

/* Resource Manager */
sprite_resource_manager_t sprite_resource_manager_create(int max_resources);
void sprite_resource_manager_destroy(sprite_resource_manager_t manager);

/* Resource Loading */
sprite_resource_t sprite_resource_load(sprite_resource_manager_t manager,
                                        const char *path,
                                        sprite_resource_type_t type);
int sprite_resource_unload(sprite_resource_manager_t manager, sprite_resource_t resource);
void *sprite_resource_get_data(sprite_resource_manager_t manager, sprite_resource_t resource);
int sprite_resource_get_info(sprite_resource_manager_t manager,
                              sprite_resource_t resource,
                              sprite_resource_info_t *info);

/* Reference Counting */
int sprite_resource_retain(sprite_resource_manager_t manager, sprite_resource_t resource);
int sprite_resource_release(sprite_resource_manager_t manager, sprite_resource_t resource);

/* Memory Pooling */
sprite_memory_pool_t sprite_memory_pool_create(int object_size, int pool_size);
void sprite_memory_pool_destroy(sprite_memory_pool_t pool);
void *sprite_memory_pool_allocate(sprite_memory_pool_t pool);
int sprite_memory_pool_deallocate(sprite_memory_pool_t pool, void *object);
int sprite_memory_pool_get_available(sprite_memory_pool_t pool);

/* Cache Control */
int sprite_resource_manager_clear_cache(sprite_resource_manager_t manager);
int sprite_resource_manager_get_loaded_count(sprite_resource_manager_t manager);
int sprite_resource_manager_get_memory_usage(sprite_resource_manager_t manager);

#endif
