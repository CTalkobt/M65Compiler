#include <far_ptr.h>
#include <dma.h>
#include <string.h>

/* ============================================================================
 * The far heap's attic RAM (Issue #305, #306): a buddy allocator
 *
 * Linked only when a program calls far_attic_init (or far_attic_enable);
 * far_attic_hook (far_attic_hook.c, which far_attic_enable calls) lets
 * far_malloc.c reach it: a program without attic RAM pays nothing, and one
 * that calls far_attic_malloc & co. itself takes no address of these
 * (calls through pointers make ln45 assume the worst). A block is a power of two, at least a granule (2^g_m),
 * and aligned to its size in attic RAM: one of 64 KB or less never crosses
 * an attic bank. A program that uses attic RAM only can call
 * far_attic_malloc / far_attic_free / far_attic_size and leave the chip
 * RAM heap (far_malloc.c) out.
 *
 * The 8 MB of attic RAM is a binary tree: node 1 the whole, nodes 2n and
 * 2n+1 the halves of node n; level l (0..g_d, g_d = 23 - g_m) has nodes
 * 2^l .. 2^(l+1)-1 of 2^(g_d-l) granules. Two bitmaps of a bit per node (the
 * program's memory, far_attic_init): g_free (a free block) and g_used (an
 * allocated one). A node in neither is split, outside the range, or part
 * of a larger block. Offsets and sizes are counted in granules (at most
 * 2^14), so the bookkeeping is 16-bit, and shifts go one bit at a time
 * (the 45GS02 shifts a bit per instruction). far_attic_init.c sets it up
 * (a member of its own: a program can keep the two in different overlays).
 * ========================================================================== */

#define ATTIC_BASE  0x8000000UL
#define ATTIC_ORDER 23              /* 8 MB */

/* (shared with far_attic_init.c) */
unsigned char *__far_attic_map, *__far_attic_used;  /* (the first 0: not enabled) */
unsigned char __far_attic_m, __far_attic_d;         /* the granule's order; the deepest level */
unsigned int __far_attic_avail_g;                   /* free granules */
#define g_free  __far_attic_map
#define g_used  __far_attic_used
#define g_m     __far_attic_m
#define g_d     __far_attic_d
#define g_avail __far_attic_avail_g

static unsigned char get(unsigned char *map, unsigned int n) {
    return map[n >> 3] & (1 << (n & 7));
}

void __far_attic_put(unsigned char *map, unsigned int n, unsigned char on) {
    unsigned char b = (unsigned char)(1 << (n & 7));
    if (on) map[n >> 3] |= b;
    else map[n >> 3] &= (unsigned char)~b;
}

static const far_ptr_t g_null;         /* (the null far pointer) */

/* granules <-> attic RAM offsets */
unsigned long __far_attic_bytes(unsigned int g) {
    unsigned long a = g;
    unsigned char i;
    for (i = g_m; i; --i) a <<= 1;
    return a;
}

/* The attic RAM's far pointer at granule g (bank $80 + the 64 KB bank) */
static far_ptr_t at(unsigned int g) {
    far_ptr_t p;
    unsigned long a = __far_attic_bytes(g);
    p.bytes[0] = (unsigned char)(0x80 | (unsigned char)(a >> 16));
    p.bytes[1] = (unsigned char)a;
    p.bytes[2] = (unsigned char)(a >> 8);
    return p;
}

far_ptr_t far_attic_malloc(unsigned long size) {
    unsigned int n = 0, end, need, b = 1, base;
    unsigned char want = g_d, l;
    if (!g_free || !size || size > __far_attic_bytes(1u << g_d)) return g_null;
    /* granules it takes, and the level of blocks that size */
    need = (unsigned int)((size + __far_attic_bytes(1) - 1) >> g_m);
    while (b < need) {
        b <<= 1;
        --want;
    }
    /* the smallest free block that holds it (a byte of none skipped) */
    l = want + 1;
    base = 2u << want;
    while (l--) {
        base >>= 1;
        for (n = base, end = base << 1; n < end; ++n) {
            if (!(n & 7) && !g_free[n >> 3]) n += 7;
            else if (get(g_free, n)) break;
        }
        if (n < end) break;
        n = 0;
    }
    if (!n) return g_null;
    __far_attic_put(g_free, n, 0);
    /* split it down: each right half free */
    while (l < want) {
        n <<= 1;
        __far_attic_put(g_free, n + 1, 1);
        ++l;
    }
    __far_attic_put(g_used, n, 1);
    g_avail -= b;
    return at((n - (1u << want)) * b);
}

/* The allocated node at a (its block's granules in *bp), or 0 */
static unsigned int attic_find(unsigned long a, unsigned int *bp) {
    unsigned int g, n, b = 1, base;
    if (!g_free || a < ATTIC_BASE) return 0;
    a -= ATTIC_BASE;
    g = (unsigned int)(a >> g_m);
    if (a >= __far_attic_bytes(1u << g_d) || __far_attic_bytes(g) != a) return 0;
    /* up from the deepest level while g is aligned to the block */
    for (base = 1u << g_d; base; base >>= 1, b <<= 1) {
        n = base + g / b;
        if (get(g_used, n)) {
            *bp = b;
            return n;
        }
        if (g & b) break;
    }
    return 0;
}

unsigned long far_attic_size(far_ptr_t p) {
    unsigned int b;
    return attic_find(far_to_linear(p), &b) ? __far_attic_bytes(b) : 0;
}

void far_attic_free(far_ptr_t p) {
    unsigned int b, n = attic_find(far_to_linear(p), &b);
    if (!n) return;
    __far_attic_put(g_used, n, 0);
    g_avail += b;
    /* merge with the buddy while it is free */
    while (n > 1 && get(g_free, n ^ 1)) {
        __far_attic_put(g_free, n ^ 1, 0);
        n >>= 1;
    }
    __far_attic_put(g_free, n, 1);
}

unsigned long far_attic_available(void) {
    return g_free ? __far_attic_bytes(g_avail) : 0;
}
