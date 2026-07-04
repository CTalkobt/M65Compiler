/* bsearch.c — Binary search for cc45 / MEGA65 */

typedef signed int (*cmpfn_t)();

cmpfn_t bs_cmp;

static signed int do_cmp(const void *a, const void *b) {
    return bs_cmp(a, b);
}

void *bsearch(const void *key, const void *base, signed int nmemb, signed int size,
              signed int (*cmpfunc)()) {
    char *arr = (char *)base;
    signed int lo = 0;
    signed int hi = nmemb - 1;
    bs_cmp = cmpfunc;

    while (lo <= hi) {
        signed int mid = lo + (hi - lo) / 2;
        signed int result = do_cmp(key, arr + mid * size);
        if (result == 0)
            return arr + mid * size;
        if (result > 0)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return (void *)0;
}
