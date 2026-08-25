/* sprite_resource_manager.c — Resource Manager Implementation */

#include "resource_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_MANAGERS 4
#define MAX_POOLS 16

typedef struct {
    char path[256];
    sprite_resource_type_t type;
    int ref_count;
    void *data;
    int size;
} resource_impl;

typedef struct {
    resource_impl *resources;
    int resource_count;
    int max_resources;
} manager_impl;

typedef struct {
    void **objects;
    int *available;
    int object_size;
    int pool_size;
    int available_count;
} pool_impl;

static manager_impl managers[MAX_MANAGERS];
static int manager_count = 0;

static pool_impl pools[MAX_POOLS];
static int pool_count = 0;

sprite_resource_manager_t sprite_resource_manager_create(int max_resources) {
    if (manager_count >= MAX_MANAGERS) return INVALID_MANAGER;

    manager_impl *mgr = &managers[manager_count];
    mgr->resources = (resource_impl *)malloc(max_resources * sizeof(resource_impl));
    mgr->resource_count = 0;
    mgr->max_resources = max_resources;

    return (sprite_resource_manager_t)(intptr_t)manager_count++;
}

void sprite_resource_manager_destroy(sprite_resource_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return;
    free(managers[idx].resources);
}

sprite_resource_t sprite_resource_load(sprite_resource_manager_t manager,
                                        const char *path,
                                        sprite_resource_type_t type) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return INVALID_RESOURCE;

    manager_impl *mgr = &managers[idx];

    for (int i = 0; i < mgr->resource_count; i++) {
        if (strcmp(mgr->resources[i].path, path) == 0) {
            mgr->resources[i].ref_count++;
            return (sprite_resource_t)(intptr_t)i;
        }
    }

    if (mgr->resource_count >= mgr->max_resources) return INVALID_RESOURCE;

    resource_impl *res = &mgr->resources[mgr->resource_count];
    strncpy(res->path, path, sizeof(res->path) - 1);
    res->type = type;
    res->ref_count = 1;
    res->data = malloc(1024);
    res->size = 1024;

    return (sprite_resource_t)(intptr_t)mgr->resource_count++;
}

int sprite_resource_unload(sprite_resource_manager_t manager, sprite_resource_t resource) {
    intptr_t m_idx = (intptr_t)manager;
    intptr_t r_idx = (intptr_t)resource;
    if (m_idx < 0 || m_idx >= manager_count || r_idx < 0) return 0;

    manager_impl *mgr = &managers[m_idx];
    if (r_idx >= mgr->resource_count) return 0;

    resource_impl *res = &mgr->resources[r_idx];
    res->ref_count--;

    if (res->ref_count <= 0) {
        free(res->data);
        res->data = NULL;
        return 1;
    }
    return 0;
}

void *sprite_resource_get_data(sprite_resource_manager_t manager, sprite_resource_t resource) {
    intptr_t m_idx = (intptr_t)manager;
    intptr_t r_idx = (intptr_t)resource;
    if (m_idx < 0 || m_idx >= manager_count || r_idx < 0) return NULL;

    manager_impl *mgr = &managers[m_idx];
    if (r_idx >= mgr->resource_count) return NULL;

    return mgr->resources[r_idx].data;
}

int sprite_resource_get_info(sprite_resource_manager_t manager,
                              sprite_resource_t resource,
                              sprite_resource_info_t *info) {
    intptr_t m_idx = (intptr_t)manager;
    intptr_t r_idx = (intptr_t)resource;
    if (m_idx < 0 || m_idx >= manager_count || r_idx < 0 || !info) return 0;

    manager_impl *mgr = &managers[m_idx];
    if (r_idx >= mgr->resource_count) return 0;

    resource_impl *res = &mgr->resources[r_idx];
    info->path = res->path;
    info->type = res->type;
    info->ref_count = res->ref_count;
    info->data = res->data;
    info->size = res->size;

    return 1;
}

int sprite_resource_retain(sprite_resource_manager_t manager, sprite_resource_t resource) {
    intptr_t m_idx = (intptr_t)manager;
    intptr_t r_idx = (intptr_t)resource;
    if (m_idx < 0 || m_idx >= manager_count || r_idx < 0) return 0;

    manager_impl *mgr = &managers[m_idx];
    if (r_idx >= mgr->resource_count) return 0;

    mgr->resources[r_idx].ref_count++;
    return 1;
}

int sprite_resource_release(sprite_resource_manager_t manager, sprite_resource_t resource) {
    return sprite_resource_unload(manager, resource);
}

sprite_memory_pool_t sprite_memory_pool_create(int object_size, int pool_size) {
    if (pool_count >= MAX_POOLS) return INVALID_POOL;

    pool_impl *pool = &pools[pool_count];
    pool->object_size = object_size;
    pool->pool_size = pool_size;
    pool->objects = (void **)malloc(pool_size * sizeof(void *));
    pool->available = (int *)malloc(pool_size * sizeof(int));
    pool->available_count = pool_size;

    for (int i = 0; i < pool_size; i++) {
        pool->objects[i] = malloc(object_size);
        pool->available[i] = 1;
    }

    return (sprite_memory_pool_t)(intptr_t)pool_count++;
}

void sprite_memory_pool_destroy(sprite_memory_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return;

    pool_impl *p = &pools[idx];
    for (int i = 0; i < p->pool_size; i++) {
        free(p->objects[i]);
    }
    free(p->objects);
    free(p->available);
}

void *sprite_memory_pool_allocate(sprite_memory_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return NULL;

    pool_impl *p = &pools[idx];
    for (int i = 0; i < p->pool_size; i++) {
        if (p->available[i]) {
            p->available[i] = 0;
            p->available_count--;
            return p->objects[i];
        }
    }
    return NULL;
}

int sprite_memory_pool_deallocate(sprite_memory_pool_t pool, void *object) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;

    pool_impl *p = &pools[idx];
    for (int i = 0; i < p->pool_size; i++) {
        if (p->objects[i] == object) {
            p->available[i] = 1;
            p->available_count++;
            return 1;
        }
    }
    return 0;
}

int sprite_memory_pool_get_available(sprite_memory_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;
    return pools[idx].available_count;
}

int sprite_resource_manager_clear_cache(sprite_resource_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int cleared = 0;

    for (int i = 0; i < mgr->resource_count; i++) {
        if (mgr->resources[i].ref_count == 0) {
            free(mgr->resources[i].data);
            cleared++;
        }
    }

    return cleared;
}

int sprite_resource_manager_get_loaded_count(sprite_resource_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;
    return managers[idx].resource_count;
}

int sprite_resource_manager_get_memory_usage(sprite_resource_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int total = 0;

    for (int i = 0; i < mgr->resource_count; i++) {
        total += mgr->resources[i].size;
    }

    return total;
}
