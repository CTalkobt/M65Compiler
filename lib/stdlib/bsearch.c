/* bsearch.c — Binary search for cc45 / MEGA65 */

typedef int (*cmpfn_t)();

cmpfn_t bs_cmp;

static int do_cmp(const void *a, const void *b) {
    return bs_cmp(a, b);
}

/* Signed positive check via high byte sign bit */
static int cmp_positive(const void *a, const void *b) {
    int v = do_cmp(a, b);
    unsigned int uv = (unsigned int)v;
    if (uv == 0) return 0;
    return (uv >> 8) < 128 ? 1 : 0;
}

void *bsearch(const void *key, const void *base, int nmemb, int size,
              int (*cmpfunc)()) {
    char *arr = (char *)base;
    int lo = 0;
    int hi = nmemb - 1;
    bs_cmp = cmpfunc;

    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int result = do_cmp(key, arr + mid * size);
        if (result == 0)
            return arr + mid * size;
        if (cmp_positive(key, arr + mid * size))
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return (void *)0;
}
