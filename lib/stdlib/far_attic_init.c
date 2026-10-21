#include <far_ptr.h>
#include <dma.h>
#include <string.h>

/* far_attic_init: the attic RAM allocator's setup (far_attic.c; a member
 * of its own, so a program can keep it out of the way, in an overlay) */

extern unsigned char *__far_attic_map, *__far_attic_used;
extern unsigned char __far_attic_m, __far_attic_d;
extern unsigned int __far_attic_avail_g;
void __far_attic_put(unsigned char *map, unsigned int n, unsigned char on);
unsigned long __far_attic_bytes(unsigned int g);

/* Code overlays kept in attic RAM (ln45 --overlay-store): from the first
 * file to the end of the last (crt0's weak defaults: bank 0, none) */
extern char __ovl_attic_bank[], __ovl_attic_first[], __ovl_attic_last_bank[], __ovl_attic_last[];

#define ATTIC_BASE  0x8000000UL
#define ATTIC_ORDER 23              /* 8 MB */

/* Writes v at a and reads it back */
static unsigned char poke(unsigned long a, unsigned char v) {
    dma_copy_far(a, (unsigned long)(unsigned int)&v, 1);
    v = 0;
    dma_copy_far((unsigned long)(unsigned int)&v, a, 1);
    return v;
}

unsigned long far_attic_init(unsigned long first, unsigned long last, unsigned char order,
                             unsigned char *meta, unsigned int size) {
    unsigned int lo, hi, o, b, top;
    unsigned char v = 0;
    __far_attic_map = 0;
    __far_attic_avail_g = 0;
    if (order < FAR_ATTIC_MIN_ORDER || order > FAR_ATTIC_MAX_ORDER || !meta ||
        size < FAR_ATTIC_META_SIZE(order)) return 0;
    if (first < ATTIC_BASE) first = ATTIC_BASE;
    if (last > ATTIC_BASE + 0x7FFFFFUL) last = ATTIC_BASE + 0x7FFFFFUL;
    /* never the overlays' store: the range is cut to the larger side */
    if ((unsigned int)__ovl_attic_bank) {
        unsigned long s = ((unsigned long)(unsigned int)__ovl_attic_bank << 16) | (unsigned int)__ovl_attic_first;
        unsigned long e = ((unsigned long)(unsigned int)__ovl_attic_last_bank << 16) | (unsigned int)__ovl_attic_last;
        if (first <= e && s <= last) {
            if (s <= first || (last > e && last - e > s - first)) first = e + 1;
            else last = s - 1;
        }
    }
    /* memory there: written and read back at both ends, the first written
     * again and the last unchanged (not a mirror) */
    if (last <= first || poke(first, 0x5A) != 0x5A || poke(last, 0xA5) != 0xA5 || poke(first, 0xC3) != 0xC3)
        return 0;
    dma_copy_far((unsigned long)(unsigned int)&v, last, 1);
    if (v != 0xA5) return 0;
    __far_attic_m = order;
    __far_attic_d = ATTIC_ORDER - order;
    memset(meta, 0, FAR_ATTIC_META_SIZE(order));
    __far_attic_used = meta + (FAR_ATTIC_META_SIZE(order) >> 1);
    /* whole granules of the range, as the largest aligned blocks */
    lo = (unsigned int)((first - ATTIC_BASE + __far_attic_bytes(1) - 1) >> order);
    hi = (unsigned int)((last - ATTIC_BASE + 1) >> order);
    top = 1u << __far_attic_d;
    for (o = lo; o < hi; o += b) {
        b = top;
        while (b > 1 && ((o & (b - 1)) || b > hi - o)) b >>= 1;
        __far_attic_put(meta, top / b + o / b, 1);
        __far_attic_avail_g += b;
    }
    if (__far_attic_avail_g) __far_attic_map = meta;
    return __far_attic_bytes(__far_attic_avail_g);
}
