/* bsearch.c — Binary search for cc45 / MEGA65 */

int (*__bsearch_cmp)();

void *bsearch(const void *key, const void *base, int nmemb, int size,
              int (*cmpfunc)()) {
    char *arr = (char *)base;
    int lo = 0;
    int hi = nmemb - 1;
    __bsearch_cmp = cmpfunc;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int result = __bsearch_cmp(key, arr + mid * size);
        if (result == 0)
            return arr + mid * size;
        if (result < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    return (void *)0;
}
