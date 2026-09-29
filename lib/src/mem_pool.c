/* mem_pool.c — Memory Pooling & Object Allocation Implementation */

#include "mem_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_POOLS 16
#define MAX_ARENAS 8

typedef struct {
    void *objects;
    int *free_list;
    unsigned char *allocated;
    int object_size;
    int max_objects;
    int num_free;
    int peak_usage;
    int alloc_count;
    int dealloc_count;
} pool_impl;

typedef struct {
    void *data;
    int size;
    int offset;
    int peak_used;
    int alloc_count;
    int *savepoints;
    int savepoint_count;
} arena_impl;

typedef struct {
    pool_impl pools[MAX_POOLS];
    int pool_count;
} pool_manager_impl;

static pool_impl pools[MAX_POOLS];
static int pool_count = 0;

static arena_impl arenas[MAX_ARENAS];
static int arena_count = 0;

/* ===== Object Pool Implementation ===== */

mem_pool_t mem_pool_create(int object_size, int max_objects) {
    if (pool_count >= MAX_POOLS) return NULL;

    pool_impl *pool = &pools[pool_count];
    pool->object_size = object_size;
    pool->max_objects = max_objects;
    pool->num_free = max_objects;
    pool->peak_usage = 0;
    pool->alloc_count = 0;
    pool->dealloc_count = 0;

    /* Allocate object storage */
    pool->objects = malloc(object_size * max_objects);
    if (!pool->objects) return NULL;

    /* Allocate free list */
    pool->free_list = (int *)malloc(sizeof(int) * max_objects);
    if (!pool->free_list) {
        free(pool->objects);
        return NULL;
    }

    /* Initialize free list (all objects free) */
    for (int i = 0; i < max_objects; i++) {
        pool->free_list[i] = i;
    }

    /* Allocate allocation bitmap */
    pool->allocated = (unsigned char *)malloc(max_objects);
    if (!pool->allocated) {
        free(pool->objects);
        free(pool->free_list);
        return NULL;
    }
    memset(pool->allocated, 0, max_objects);

    return (mem_pool_t)(intptr_t)pool_count++;
}

void mem_pool_destroy(mem_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return;

    pool_impl *p = &pools[idx];
    free(p->objects);
    free(p->free_list);
    free(p->allocated);
}

void *mem_pool_alloc(mem_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return NULL;

    pool_impl *p = &pools[idx];
    if (p->num_free == 0) return NULL;

    /* Get next free object from free list */
    int obj_idx = p->free_list[p->num_free - 1];
    p->num_free--;
    p->allocated[obj_idx] = 1;
    p->alloc_count++;

    /* Update peak usage */
    int used = p->max_objects - p->num_free;
    if (used > p->peak_usage) {
        p->peak_usage = used;
    }

    /* Return pointer to object */
    return (void *)((char *)p->objects + obj_idx * p->object_size);
}

int mem_pool_free(mem_pool_t pool, void *object) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;

    pool_impl *p = &pools[idx];
    if (!object) return 0;

    /* Calculate object index */
    intptr_t offset = (char *)object - (char *)p->objects;
    if (offset < 0 || offset % p->object_size != 0) return 0;

    int obj_idx = offset / p->object_size;
    if (obj_idx < 0 || obj_idx >= p->max_objects) return 0;
    if (!p->allocated[obj_idx]) return 0;

    /* Return object to free list */
    p->allocated[obj_idx] = 0;
    p->free_list[p->num_free] = obj_idx;
    p->num_free++;
    p->dealloc_count++;

    return 1;
}

mem_pool_stats_t mem_pool_get_stats(mem_pool_t pool) {
    mem_pool_stats_t stats = {0};
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return stats;

    pool_impl *p = &pools[idx];
    stats.total_objects = p->max_objects;
    stats.free_objects = p->num_free;
    stats.allocated_objects = p->max_objects - p->num_free;
    stats.object_size = p->object_size;
    stats.peak_usage = p->peak_usage;
    stats.allocation_count = p->alloc_count;
    stats.deallocation_count = p->dealloc_count;

    return stats;
}

int mem_pool_reset(mem_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;

    pool_impl *p = &pools[idx];
    p->num_free = p->max_objects;
    memset(p->allocated, 0, p->max_objects);

    /* Rebuild free list */
    for (int i = 0; i < p->max_objects; i++) {
        p->free_list[i] = i;
    }

    return 1;
}

int mem_pool_owns(mem_pool_t pool, void *object) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;

    pool_impl *p = &pools[idx];
    if (!object) return 0;

    intptr_t offset = (char *)object - (char *)p->objects;
    return offset >= 0 && offset < (intptr_t)(p->object_size * p->max_objects) &&
           offset % p->object_size == 0;
}

int mem_pool_available(mem_pool_t pool) {
    intptr_t idx = (intptr_t)pool;
    if (idx < 0 || idx >= pool_count) return 0;
    return pools[idx].num_free;
}

/* ===== Linear Arena Implementation ===== */

mem_arena_t mem_arena_create(int size) {
    if (arena_count >= MAX_ARENAS) return NULL;

    arena_impl *arena = &arenas[arena_count];
    arena->data = malloc(size);
    if (!arena->data) return NULL;

    arena->size = size;
    arena->offset = 0;
    arena->peak_used = 0;
    arena->alloc_count = 0;
    arena->savepoints = (int *)malloc(sizeof(int) * 16);  /* Max 16 savepoints */
    arena->savepoint_count = 0;

    return (mem_arena_t)(intptr_t)arena_count++;
}

void mem_arena_destroy(mem_arena_t arena) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return;

    arena_impl *a = &arenas[idx];
    free(a->data);
    free(a->savepoints);
}

void *mem_arena_alloc(mem_arena_t arena, int size, int alignment) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return NULL;

    arena_impl *a = &arenas[idx];

    /* Align offset */
    int aligned_offset = a->offset;
    int remainder = aligned_offset % alignment;
    if (remainder != 0) {
        aligned_offset += alignment - remainder;
    }

    /* Check if we have space */
    if (aligned_offset + size > a->size) return NULL;

    /* Update peak usage */
    if (aligned_offset + size > a->peak_used) {
        a->peak_used = aligned_offset + size;
    }

    void *ptr = (char *)a->data + aligned_offset;
    a->offset = aligned_offset + size;
    a->alloc_count++;

    return ptr;
}

int mem_arena_reset(mem_arena_t arena) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return 0;

    arena_impl *a = &arenas[idx];
    a->offset = 0;
    a->savepoint_count = 0;

    return 1;
}

mem_arena_stats_t mem_arena_get_stats(mem_arena_t arena) {
    mem_arena_stats_t stats = {0};
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return stats;

    arena_impl *a = &arenas[idx];
    stats.total_size = a->size;
    stats.used_size = a->offset;
    stats.free_size = a->size - a->offset;
    stats.peak_used = a->peak_used;
    stats.allocation_count = a->alloc_count;

    return stats;
}

int mem_arena_offset(mem_arena_t arena) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return 0;
    return arenas[idx].offset;
}

int mem_arena_save(mem_arena_t arena) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return -1;

    arena_impl *a = &arenas[idx];
    if (a->savepoint_count >= 16) return -1;

    a->savepoints[a->savepoint_count] = a->offset;
    return a->savepoint_count++;
}

void mem_arena_restore(mem_arena_t arena, int savepoint) {
    intptr_t idx = (intptr_t)arena;
    if (idx < 0 || idx >= arena_count) return;

    arena_impl *a = &arenas[idx];
    if (savepoint >= 0 && savepoint < a->savepoint_count) {
        a->offset = a->savepoints[savepoint];
        a->savepoint_count = savepoint;
    }
}

/* ===== Pool Manager Implementation ===== */

mem_pool_manager_t mem_pool_manager_create(void) {
    pool_manager_impl *mgr = (pool_manager_impl *)malloc(sizeof(pool_manager_impl));
    if (!mgr) return NULL;

    mgr->pool_count = 0;
    return (mem_pool_manager_t)mgr;
}

int mem_pool_manager_register(mem_pool_manager_t mgr,
                               int type_id,
                               int object_size,
                               int max_objects) {
    pool_manager_impl *m = (pool_manager_impl *)mgr;
    if (!m || type_id < 0 || type_id >= MAX_POOLS) return 0;

    mem_pool_t pool = mem_pool_create(object_size, max_objects);
    if (!pool) return 0;

    return 1;
}

void *mem_pool_manager_alloc(mem_pool_manager_t mgr, int type_id) {
    if (!mgr || type_id < 0 || type_id >= MAX_POOLS) return NULL;
    return mem_pool_alloc((mem_pool_t)(intptr_t)type_id);
}

int mem_pool_manager_free(mem_pool_manager_t mgr, int type_id, void *object) {
    if (!mgr || type_id < 0 || type_id >= MAX_POOLS) return 0;
    return mem_pool_free((mem_pool_t)(intptr_t)type_id, object);
}

mem_pool_stats_t mem_pool_manager_stats(mem_pool_manager_t mgr, int type_id) {
    mem_pool_stats_t stats = {0};
    if (!mgr || type_id < 0 || type_id >= MAX_POOLS) return stats;
    return mem_pool_get_stats((mem_pool_t)(intptr_t)type_id);
}

void mem_pool_manager_destroy(mem_pool_manager_t mgr) {
    if (!mgr) return;
    free(mgr);
}
