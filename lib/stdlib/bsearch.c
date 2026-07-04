/* bsearch.c — Binary search for cc45 / MEGA65 */

typedef int (*cmpfn_t)();

cmpfn_t bs_cmp;

static int do_cmp(const void *a, const void *b) {
    return bs_cmp(a, b);
}

/* Signed negative check using bit 15 */
static int is_negative(int v) {
    return ((unsigned int)v & 0x8000) ? 1 : 0;
}

void *bsearch(const void *key, const void *base, int nmemb, int size,
              int (*cmpfunc)()) {
    char *arr = (char *)base;
    int lo = 0;
    int hi = nmemb - 1;
    bs_cmp = cmpfunc;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int result = do_cmp(key, arr + mid * size);
        if (result == 0)
            return arr + mid * size;
        if (is_negative(result))
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return (void *)0;
}
