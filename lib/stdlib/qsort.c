/* qsort.c — qsort and bsearch (<stdlib.h>)
 *
 * The algorithms are in assembly (stdlib_asm/sort.s45). The comparison
 * function is called here, through __sort_cmp: compiled in the library's
 * calling convention, it calls the program's function through the pointer
 * in the same convention (ln45 bridges the assembly's call of __sort_cmp,
 * not a call through a pointer).
 */
#include <stdlib.h>

static int (*__sort_fn)();

int __sort_cmp(const void *a, const void *b) {
    return __sort_fn(a, b);
}

__fastcall__ void __qsort_core(void *base, int nmemb, int size);
__fastcall__ void *__bsearch_core(const void *key, const void *base, int nmemb, int size);

void qsort(void *base, int nmemb, int size, int (*cmpfunc)()) {
    __sort_fn = cmpfunc;
    __qsort_core(base, nmemb, size);
}

void *bsearch(const void *key, const void *base, int nmemb, int size, int (*cmpfunc)()) {
    __sort_fn = cmpfunc;
    return __bsearch_core(key, base, nmemb, size);
}
