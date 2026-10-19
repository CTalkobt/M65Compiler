#include <far_ptr.h>
#include <far_ops.h>
#include <string.h>

/* ============================================================================
 * The far heap's attic RAM (Issue #305, #306): a buddy allocator
 *
 * Linked only when a program calls far_attic_init (or far_attic_enable),
 * which hooks it into far_malloc.c (__far_attic_*): a program without attic
 * RAM pays nothing. A block is a power of two, at least a granule (2^g_m),
 * and aligned to its size in attic RAM: one of 64 KB or less never crosses
 * an attic bank.
 *
 * The 8 MB of attic RAM is a binary tree: node 1 the whole, nodes 2n and
 * 2n+1 the halves of node n; level l (0..g_d, g_d = 23 - g_m) has nodes
 * 2^l .. 2^(l+1)-1 of 2^(23-l) bytes. Two bitmaps of a bit per node (the
 * program's memory, far_attic_init): g_free (a free block) and g_used (an
 * allocated one). A node in neither is split, outside the range, or part
 * of a larger block.
 * ========================================================================== */

extern far_ptr_t (*__far_attic_alloc)(unsigned long size);
extern unsigned long (*__far_attic_size)(unsigned long at);
extern void (*__far_attic_free)(unsigned long at);
extern unsigned long (*__far_attic_avail)(void);

#define ATTIC_BASE  0x8000000UL
#define ATTIC_ORDER 23              /* 8 MB */

static unsigned char *g_free, *g_used;
static unsigned char g_m, g_d;      /* the granule's order; the deepest level */
static unsigned char g_on;

static unsigned char get(unsigned char *map, unsigned int n) {
    return map[n >> 3] & (1 << (n & 7));
}

static void set(unsigned char *map, unsigned int n) {
    map[n >> 3] |= (unsigned char)(1 << (n & 7));
}

static void clear(unsigned char *map, unsigned int n) {
    map[n >> 3] &= (unsigned char)~(1 << (n & 7));
}

/* A free node of level l, or 0 */
static unsigned int find_free(unsigned char l) {
    unsigned int n = 1u << l, end = n << 1, b;
    if (l >= 3) {
        /* whole bytes: the level's bits start on one */
        for (b = n >> 3; b < end >> 3; ++b)
            if (g_free[b]) {
                n = b << 3;
                while (!get(g_free, n)) ++n;
                return n;
            }
        return 0;
    }
    for (; n < end; ++n)
        if (get(g_free, n)) return n;
    return 0;
}

static far_ptr_t attic_malloc(unsigned long size) {
    unsigned char k, want, l;
    unsigned int n = 0;
    if (!g_on || !size || size > (1UL << ATTIC_ORDER)) return far_make(BANK0, 0);
    for (k = g_m; (1UL << k) < size; ++k) {}
    want = ATTIC_ORDER - k;
    /* the smallest free block that holds it */
    l = want + 1;
    while (l-- && !(n = find_free(l))) {}
    if (!n) return far_make(BANK0, 0);
    clear(g_free, n);
    /* split it down: each right half free */
    while (l < want) {
        n <<= 1;
        set(g_free, n + 1);
        ++l;
    }
    set(g_used, n);
    return far_of_linear(ATTIC_BASE + ((unsigned long)(n - (1u << want)) << k));
}

/* The allocated node at a (its level in *lp), or 0 */
static unsigned int attic_find(unsigned long a, unsigned char *lp) {
    unsigned long o;
    unsigned char l, k;
    unsigned int n;
    if (!g_on || a < ATTIC_BASE || a >= ATTIC_BASE + (1UL << ATTIC_ORDER)) return 0;
    o = a - ATTIC_BASE;
    l = g_d + 1;
    while (l--) {
        k = ATTIC_ORDER - l;
        if (o & ((1UL << k) - 1)) break;    /* (a larger block: not aligned either) */
        n = (1u << l) + (unsigned int)(o >> k);
        if (get(g_used, n)) {
            *lp = l;
            return n;
        }
    }
    return 0;
}

static unsigned long attic_size(unsigned long a) {
    unsigned char l;
    return attic_find(a, &l) ? 1UL << (ATTIC_ORDER - l) : 0;
}

static void attic_free(unsigned long a) {
    unsigned char l;
    unsigned int n = attic_find(a, &l);
    if (!n) return;
    clear(g_used, n);
    /* merge with the buddy while it is free */
    while (l && get(g_free, n ^ 1)) {
        clear(g_free, n ^ 1);
        n >>= 1;
        --l;
    }
    set(g_free, n);
}

unsigned long far_attic_available(void) {
    unsigned long sum = 0;
    unsigned int n, end;
    unsigned char l;
    if (!g_on) return 0;
    for (l = 0; l <= g_d; ++l) {
        end = 2u << l;
        for (n = 1u << l; n < end; ++n) {
            if (!(n & 7) && n + 8 <= end && !g_free[n >> 3]) {
                n += 7;                     /* a byte with none */
                continue;
            }
            if (get(g_free, n)) sum += 1UL << (ATTIC_ORDER - l);
        }
    }
    return sum;
}

/* Is there memory at a? (a write read back, a second address not the same
 * byte: no RAM reads back what was written, or mirrors) */
static int attic_probe(unsigned long first, unsigned long last) {
    unsigned char a = 0x5A, b = 0xA5, r = 0;
    far_ptr_t pf = far_of_linear(first), pl = far_of_linear(last);
    far_memcpy_from_local(pf, &a, 1);
    far_memcpy_from_local(pl, &b, 1);
    far_memcpy_to_local(&r, pf, 1);
    if (r != 0x5A) return 0;
    far_memcpy_to_local(&r, pl, 1);
    if (r != 0xA5) return 0;
    a = 0xC3;
    far_memcpy_from_local(pf, &a, 1);
    far_memcpy_to_local(&r, pl, 1);
    return r == 0xA5;
}

unsigned long far_attic_init(unsigned long first, unsigned long last, unsigned char order,
                             unsigned char *meta, unsigned int size) {
    unsigned long lo, hi, o, total;
    unsigned char k;
    __far_attic_alloc = attic_malloc;
    __far_attic_size = attic_size;
    __far_attic_free = attic_free;
    __far_attic_avail = far_attic_available;
    g_on = 0;
    if (order < FAR_ATTIC_MIN_ORDER || order > FAR_ATTIC_MAX_ORDER || !meta ||
        size < FAR_ATTIC_META_SIZE(order)) return 0;
    if (first < ATTIC_BASE) first = ATTIC_BASE;
    if (last > ATTIC_BASE + (1UL << ATTIC_ORDER) - 1) last = ATTIC_BASE + (1UL << ATTIC_ORDER) - 1;
    if (last <= first || !attic_probe(first, last)) return 0;
    g_m = order;
    g_d = ATTIC_ORDER - order;
    g_free = meta;
    g_used = meta + (FAR_ATTIC_META_SIZE(order) >> 1);
    memset(meta, 0, FAR_ATTIC_META_SIZE(order));
    /* whole granules of the range, as the largest aligned blocks */
    lo = (first - ATTIC_BASE + (1UL << order) - 1) & ~((1UL << order) - 1);
    hi = (last - ATTIC_BASE + 1) & ~((1UL << order) - 1);
    total = 0;
    for (o = lo; o < hi; o += 1UL << k) {
        k = ATTIC_ORDER;
        while (k > order && ((o & ((1UL << k) - 1)) || o + (1UL << k) > hi)) --k;
        set(g_free, (1u << (ATTIC_ORDER - k)) + (unsigned int)(o >> k));
        total += 1UL << k;
    }
    g_on = total != 0;
    return total;
}
