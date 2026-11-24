/* mem_pool.c — object pools, linear arenas, a pool manager (<mem_pool.h>, #645)
 *
 * A pool: one malloc block, its header, then the free-slot stack (a byte
 * per object), a bitmap of the slots in use, then the objects. An arena: a
 * header, then its bytes.
 */

#include <mem_pool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char *objects;
    unsigned char *free_stack;  /* the free slots, top at [nfree - 1] */
    unsigned char *used;        /* a bit per slot */
    unsigned int size, count, nfree;
    mem_pool_stats_t st;
} mp_pool;

mem_pool_t mem_pool_create(int object_size, int max_objects) {
    unsigned int n = (unsigned int)max_objects, sz = (unsigned int)object_size;
    unsigned int bits = (n + 7) / 8;
    mp_pool *p;
    if (n == 0 || n > 256 || sz == 0) return NULL;
    if ((unsigned long)sz * n > 60000UL) return NULL;
    p = (mp_pool *)malloc(sizeof(mp_pool) + n + bits + sz * n);
    if (!p) return NULL;
    p->free_stack = (unsigned char *)(p + 1);
    p->used = p->free_stack + n;
    p->objects = p->used + bits;
    p->size = sz;
    p->count = n;
    memset(&p->st, 0, sizeof p->st);
    p->st.total_objects = (int)n;
    p->st.object_size = (int)sz;
    mem_pool_reset(p);
    return p;
}

void mem_pool_destroy(mem_pool_t pool) {
    free(pool);
}

int mem_pool_reset(mem_pool_t pool) {
    mp_pool *p = (mp_pool *)pool;
    unsigned int i;
    if (!p) return 0;
    /* (slot 0 on top: handed out first) */
    for (i = 0; i < p->count; i++) p->free_stack[i] = (unsigned char)(p->count - 1 - i);
    p->nfree = p->count;
    memset(p->used, 0, (p->count + 7) / 8);
    p->st.allocated_objects = 0;
    p->st.free_objects = (int)p->count;
    return 1;
}

void *mem_pool_alloc(mem_pool_t pool) {
    mp_pool *p = (mp_pool *)pool;
    unsigned char k;
    if (!p || !p->nfree) return NULL;
    k = p->free_stack[--p->nfree];
    p->used[k >> 3] |= (unsigned char)(1 << (k & 7));
    p->st.allocated_objects++;
    p->st.free_objects--;
    p->st.allocation_count++;
    if (p->st.allocated_objects > p->st.peak_usage) p->st.peak_usage = p->st.allocated_objects;
    return p->objects + (unsigned int)k * p->size;
}

/* the object's slot, or -1 when it is not one of the pool's */
static int mp_slot(mp_pool *p, void *object) {
    unsigned char *o = (unsigned char *)object;
    unsigned int off;
    if (!p || o < p->objects) return -1;
    off = (unsigned int)(o - p->objects);
    if (off % p->size || off / p->size >= p->count) return -1;
    return (int)(off / p->size);
}

int mem_pool_owns(mem_pool_t pool, void *object) {
    return mp_slot((mp_pool *)pool, object) >= 0;
}

int mem_pool_free(mem_pool_t pool, void *object) {
    mp_pool *p = (mp_pool *)pool;
    signed int k = mp_slot(p, object);
    unsigned char bit;
    if (k < 0) return 0;
    bit = (unsigned char)(1 << (k & 7));
    if (!(p->used[k >> 3] & bit)) return 0;         /* free already */
    p->used[k >> 3] &= (unsigned char)~bit;
    p->free_stack[p->nfree++] = (unsigned char)k;
    p->st.allocated_objects--;
    p->st.free_objects++;
    p->st.deallocation_count++;
    return 1;
}

mem_pool_stats_t mem_pool_get_stats(mem_pool_t pool) {
    mem_pool_stats_t z;
    if (pool) return ((mp_pool *)pool)->st;
    memset(&z, 0, sizeof z);
    return z;
}

int mem_pool_available(mem_pool_t pool) {
    return pool ? (int)((mp_pool *)pool)->nfree : 0;
}

/* ===== Linear arena ===== */

typedef struct {
    unsigned char *base;
    unsigned int size, used;
    mem_arena_stats_t st;
} mp_arena;

mem_arena_t mem_arena_create(int size) {
    mp_arena *a;
    if ((unsigned int)size == 0) return NULL;
    a = (mp_arena *)malloc(sizeof(mp_arena) + (unsigned int)size);
    if (!a) return NULL;
    a->base = (unsigned char *)(a + 1);
    a->size = (unsigned int)size;
    a->used = 0;
    memset(&a->st, 0, sizeof a->st);
    a->st.total_size = size;
    a->st.free_size = size;
    return a;
}

void mem_arena_destroy(mem_arena_t arena) {
    free(arena);
}

static void ma_set(mp_arena *a, unsigned int used) {
    a->used = used;
    a->st.used_size = (int)used;
    a->st.free_size = (int)(a->size - used);
    if (a->st.used_size > a->st.peak_used) a->st.peak_used = a->st.used_size;
}

void *mem_arena_alloc(mem_arena_t arena, int size, int alignment) {
    mp_arena *a = (mp_arena *)arena;
    unsigned int al = (unsigned int)alignment, start, end;
    if (!a) return NULL;
    if (al != 2 && al != 4 && al != 8 && al != 16) al = 1;
    /* (the address aligned, not the offset) */
    start = (unsigned int)(a->base + a->used);
    start = (start + al - 1) & ~(al - 1);
    start -= (unsigned int)a->base;
    end = start + (unsigned int)size;
    if (end < start || end > a->size) return NULL;
    ma_set(a, end);
    a->st.allocation_count++;
    return a->base + start;
}

int mem_arena_reset(mem_arena_t arena) {
    if (!arena) return 0;
    ma_set((mp_arena *)arena, 0);
    return 1;
}

mem_arena_stats_t mem_arena_get_stats(mem_arena_t arena) {
    mem_arena_stats_t z;
    if (arena) return ((mp_arena *)arena)->st;
    memset(&z, 0, sizeof z);
    return z;
}

int mem_arena_offset(mem_arena_t arena) {
    return arena ? (int)((mp_arena *)arena)->used : 0;
}

int mem_arena_save(mem_arena_t arena) {
    return mem_arena_offset(arena);
}

void mem_arena_restore(mem_arena_t arena, int savepoint) {
    mp_arena *a = (mp_arena *)arena;
    if (a && (unsigned int)savepoint <= a->used) ma_set(a, (unsigned int)savepoint);
}

/* ===== Pool manager ===== */

typedef struct {
    int type_id;
    mem_pool_t pool;
} mp_entry;

typedef struct {
    mp_entry *e;
    unsigned int n;
} mp_manager;

mem_pool_manager_t mem_pool_manager_create(void) {
    return calloc(1, sizeof(mp_manager));
}

static mem_pool_t mm_find(mem_pool_manager_t mgr, int type_id) {
    mp_manager *m = (mp_manager *)mgr;
    unsigned int i;
    if (!m) return NULL;
    for (i = 0; i < m->n; i++)
        if (m->e[i].type_id == type_id) return m->e[i].pool;
    return NULL;
}

int mem_pool_manager_register(mem_pool_manager_t mgr, int type_id, int object_size, int max_objects) {
    mp_manager *m = (mp_manager *)mgr;
    mp_entry *e;
    mem_pool_t p;
    if (!m || mm_find(mgr, type_id)) return 0;
    p = mem_pool_create(object_size, max_objects);
    if (!p) return 0;
    e = (mp_entry *)realloc(m->e, (m->n + 1) * sizeof(mp_entry));
    if (!e) {
        mem_pool_destroy(p);
        return 0;
    }
    m->e = e;
    m->e[m->n].type_id = type_id;
    m->e[m->n].pool = p;
    m->n++;
    return 1;
}

void *mem_pool_manager_alloc(mem_pool_manager_t mgr, int type_id) {
    return mem_pool_alloc(mm_find(mgr, type_id));
}

int mem_pool_manager_free(mem_pool_manager_t mgr, int type_id, void *object) {
    return mem_pool_free(mm_find(mgr, type_id), object);
}

mem_pool_stats_t mem_pool_manager_stats(mem_pool_manager_t mgr, int type_id) {
    return mem_pool_get_stats(mm_find(mgr, type_id));
}

void mem_pool_manager_destroy(mem_pool_manager_t mgr) {
    mp_manager *m = (mp_manager *)mgr;
    unsigned int i;
    if (!m) return;
    for (i = 0; i < m->n; i++) mem_pool_destroy(m->e[i].pool);
    free(m->e);
    free(m);
}
