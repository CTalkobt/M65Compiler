/* test_mem_pool_xemu.c — <mem_pool.h> (#645)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] pool create (0, 257 objects, 0 size: NULL), alloc until empty
 *       (distinct, object_size apart, writable), then NULL; available
 *   [1] free (one not the pool's, inside an object, twice: 0), the freed
 *       one handed out again; owns
 *   [2] stats (totals, peak, counts), reset (all free, peak kept)
 *   [3] arena: alloc in order, alignment of the address (2, 4, 16; 3 as
 *       1), too big: NULL, offset, save / restore, reset, stats
 *   [4] manager: register (an id twice: 0), alloc / free per type, stats,
 *       an unknown type (NULL / 0)
 *   [5] destroy (pools, arena, manager) and malloc gets the memory back
 *   [6] $AA sentinel
 */
#include <mem_pool.h>
#include <stdlib.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

int main(void) {
    mem_pool_t p;
    mem_arena_t a;
    mem_pool_manager_t m;
    mem_pool_stats_t st;
    mem_arena_stats_t as;
    unsigned char *o[5], *x, *y, *z;
    unsigned int i;
    int ok;
    void *probe;
    OUT[6] = 0;

    ok = mem_pool_create(10, 0) == NULL && mem_pool_create(10, 257) == NULL && mem_pool_create(0, 4) == NULL;
    p = mem_pool_create(10, 5);
    ok = ok && p != NULL && mem_pool_available(p) == 5;
    for (i = 0; i < 5; i++) {
        o[i] = (unsigned char *)mem_pool_alloc(p);
        ok = ok && o[i] != NULL;
        if (o[i]) memset(o[i], (int)i + 1, 10);
    }
    for (i = 1; i < 5; i++) ok = ok && o[i] - o[i - 1] == 10;
    ok = ok && mem_pool_alloc(p) == NULL && mem_pool_available(p) == 0 && o[4][9] == 5;
    OUT[0] = ok;

    ok = mem_pool_free(p, o[2]) == 1 && mem_pool_free(p, o[2]) == 0 && mem_pool_free(p, o[1] + 3) == 0 &&
         mem_pool_free(p, &ok) == 0 && mem_pool_available(p) == 1;
    ok = ok && mem_pool_alloc(p) == o[2] && mem_pool_owns(p, o[4]) && !mem_pool_owns(p, o[4] + 1) &&
         !mem_pool_owns(p, &ok) && o[1][0] == 2;
    OUT[1] = ok;

    st = mem_pool_get_stats(p);
    ok = st.total_objects == 5 && st.allocated_objects == 5 && st.free_objects == 0 && st.object_size == 10 &&
         st.peak_usage == 5 && st.allocation_count == 6 && st.deallocation_count == 1;
    ok = ok && mem_pool_reset(p) == 1 && mem_pool_available(p) == 5;
    st = mem_pool_get_stats(p);
    ok = ok && st.allocated_objects == 0 && st.free_objects == 5 && st.peak_usage == 5;
    OUT[2] = ok;

    a = mem_arena_create(100);
    x = (unsigned char *)mem_arena_alloc(a, 3, 1);
    y = (unsigned char *)mem_arena_alloc(a, 4, 4);
    z = (unsigned char *)mem_arena_alloc(a, 1, 16);
    ok = a != NULL && x != NULL && ((unsigned int)y & 3) == 0 && y >= x + 3 && ((unsigned int)z & 15) == 0 &&
         z >= y + 4;
    ok = ok && mem_arena_offset(a) == (int)(z + 1 - x);
    {
        int sp = mem_arena_save(a);
        unsigned char *w = (unsigned char *)mem_arena_alloc(a, 5, 3);   /* (3: as 1) */
        ok = ok && w == z + 1 && mem_arena_alloc(a, 200, 1) == NULL && mem_arena_offset(a) == sp + 5;
        mem_arena_restore(a, sp);
        ok = ok && mem_arena_offset(a) == sp && mem_arena_alloc(a, 2, 2) != NULL;
    }
    as = mem_arena_get_stats(a);
    ok = ok && as.total_size == 100 && as.allocation_count == 5 && as.used_size == mem_arena_offset(a) &&
         as.free_size == 100 - as.used_size && as.peak_used >= as.used_size;
    ok = ok && mem_arena_reset(a) == 1 && mem_arena_offset(a) == 0 && mem_arena_alloc(a, 1, 1) == x;
    OUT[3] = ok;

    m = mem_pool_manager_create();
    ok = m != NULL && mem_pool_manager_register(m, 7, 4, 3) == 1 && mem_pool_manager_register(m, 9, 20, 2) == 1 &&
         mem_pool_manager_register(m, 7, 8, 8) == 0;
    x = (unsigned char *)mem_pool_manager_alloc(m, 7);
    y = (unsigned char *)mem_pool_manager_alloc(m, 9);
    ok = ok && x && y && mem_pool_manager_alloc(m, 8) == NULL && mem_pool_manager_free(m, 9, x) == 0 &&
         mem_pool_manager_free(m, 7, x) == 1 && mem_pool_manager_free(m, 3, y) == 0;
    st = mem_pool_manager_stats(m, 9);
    ok = ok && st.object_size == 20 && st.total_objects == 2 && st.allocated_objects == 1;
    st = mem_pool_manager_stats(m, 8);
    ok = ok && st.total_objects == 0;
    OUT[4] = ok;

    /* all of it freed: the same memory to malloc again */
    mem_pool_destroy(p);
    mem_arena_destroy(a);
    mem_pool_manager_destroy(m);
    probe = malloc(400);
    ok = probe != NULL;
    free(probe);
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
