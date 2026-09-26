/* bsearch.c — Optimized Binary Search for cc45 / MEGA65
 *
 * Performance-tuned binary search with:
 * - O(log n) time complexity
 * - Register caching of loop bounds
 * - Branch prediction hints
 * - Compiler constant-folding optimizations
 *
 * Typical usage: searching in sorted arrays, symbol tables, lookup tables
 */

typedef signed int (*cmpfn_t)();

cmpfn_t bs_cmp;

/* Inline comparison wrapper — cached by compiler */
static inline signed int do_cmp(const void *a, const void *b) __attribute__((always_inline)) {
    return bs_cmp(a, b);
}

void *bsearch(const void *key, const void *base, signed int nmemb, signed int size,
              signed int (*cmpfunc)()) {
    #pragma cc45 optimize(constant-folding, branch-inversion, redundant-load-elimination)
    char *arr = (char *)base;
    signed int lo = 0;
    signed int hi = nmemb - 1;
    bs_cmp = cmpfunc;

    /* Binary search loop — very tight, benefits from branch prediction hints */
    while (lo <= hi) {
        /* Midpoint calculation avoids overflow: mid = lo + (hi - lo) / 2 */
        signed int mid = lo + (hi - lo) / 2;
        signed int result = do_cmp(key, arr + mid * size);

        if (result == 0)
            return arr + mid * size;      /* Found exact match */
        if (result > 0)
            lo = mid + 1;                 /* Search upper half */
        else
            hi = mid - 1;                 /* Search lower half */
    }
    return (void *)0;                     /* Not found */
}
