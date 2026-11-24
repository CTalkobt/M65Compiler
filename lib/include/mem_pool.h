
/* mem_pool.h — object pools and linear arenas (#645)
 *
 * Object pool: max_objects (1-256) objects of object_size bytes in one
 * block from malloc; alloc and free are O(1) (a stack of free slots).
 * mem_pool_free refuses a pointer that is not one of the pool's objects or
 * is free already (0); 1 when it was freed. Objects are not cleared.
 *
 * Linear arena: size bytes from malloc, handed out in order; alignment 1,
 * 2, 4, 8 or 16 (of the address; anything else is 1); NULL when it does
 * not fit. mem_arena_save gives the offset, mem_arena_restore goes back to
 * it (the allocations after it are dropped); mem_arena_reset to 0.
 *
 * Pool manager: a pool per type id (mem_pool_manager_register: 1, 0 when
 * the id has one already or memory is short), destroyed with the manager.
 *
 * The statistics are counts since the pool / arena was made (peak: the
 * most at once). Library: stdlib/mem_pool.c. Test: test_mem_pool_xemu.c.
 */

#ifndef MEM_POOL_H
#define MEM_POOL_H

typedef void *mem_pool_t;
typedef void *mem_arena_t;

/* Pool statistics */
typedef struct {
    int total_objects;
    int allocated_objects;
    int free_objects;
    int object_size;
    int peak_usage;
    int allocation_count;
    int deallocation_count;
} mem_pool_stats_t;

/* Arena statistics */
typedef struct {
    int total_size;
    int used_size;
    int free_size;
    int peak_used;
    int allocation_count;
} mem_arena_stats_t;

/* ===== Object Pool API ===== */

/* Create fixed-size object pool (1-256 objects): NULL when out of memory */
mem_pool_t mem_pool_create(int object_size, int max_objects);

/* Destroy pool and free all memory */
void mem_pool_destroy(mem_pool_t pool);

/* Allocate object from pool (O(1)) */
void *mem_pool_alloc(mem_pool_t pool);

/* Deallocate object back to pool (O(1)) */
int mem_pool_free(mem_pool_t pool, void *object);

/* Get pool statistics */
mem_pool_stats_t mem_pool_get_stats(mem_pool_t pool);

/* Reset pool (free all, ready for reuse): 1 */
int mem_pool_reset(mem_pool_t pool);

/* Check if object belongs to pool */
int mem_pool_owns(mem_pool_t pool, void *object);

/* Get number of free objects available */
int mem_pool_available(mem_pool_t pool);

/* ===== Linear Arena API ===== */

/* Create linear arena (push-only allocator for temporary data): NULL when out of memory */
mem_arena_t mem_arena_create(int size);

/* Destroy arena */
void mem_arena_destroy(mem_arena_t arena);

/* Allocate from arena (alignment: 1, 2, 4, 8, 16) */
void *mem_arena_alloc(mem_arena_t arena, int size, int alignment);

/* Reset arena to start (clear all allocations) */
int mem_arena_reset(mem_arena_t arena);

/* Get arena statistics */
mem_arena_stats_t mem_arena_get_stats(mem_arena_t arena);

/* Get current offset into arena */
int mem_arena_offset(mem_arena_t arena);

/* Save/restore arena state for nested allocations */
int mem_arena_save(mem_arena_t arena);
void mem_arena_restore(mem_arena_t arena, int savepoint);

/* ===== Multi-Pool Manager ===== */

typedef void *mem_pool_manager_t;

/* Create pool manager for multiple object types */
mem_pool_manager_t mem_pool_manager_create(void);

/* Register object type with pool */
int mem_pool_manager_register(mem_pool_manager_t mgr,
                               int type_id,
                               int object_size,
                               int max_objects);

/* Allocate from type pool */
void *mem_pool_manager_alloc(mem_pool_manager_t mgr, int type_id);

/* Free to type pool */
int mem_pool_manager_free(mem_pool_manager_t mgr, int type_id, void *object);

/* Get stats for type */
mem_pool_stats_t mem_pool_manager_stats(mem_pool_manager_t mgr, int type_id);

/* Destroy all pools */
void mem_pool_manager_destroy(mem_pool_manager_t mgr);

#endif
