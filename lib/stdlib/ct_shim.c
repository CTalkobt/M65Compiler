/* ct_shim.c — the container library's comparator call (#409)
 *
 * The containers are assembly (stdlib_asm/); a comparison or hash function
 * the program gives is called here, through __ct_cmp: compiled in the
 * library's calling convention, it calls the program's function through the
 * pointer in the same convention (ln45 bridges the assembly's call of
 * __ct_cmp, not a call through a pointer), as qsort's __sort_cmp does. The
 * container stores the function in __ct_cmpf before each operation. A
 * comparison function must not use the same container library.
 */
int (*__ct_cmpf)(const void *, const void *);

int __ct_cmp(const void *a, const void *b) {
    return __ct_cmpf(a, b);
}
