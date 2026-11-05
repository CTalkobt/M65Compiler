/* ct_hash.c — the container library's hash function call (#407)
 *
 * As ct_shim.c's __ct_cmp: <hashmap.h>'s assembly stores the program's hash
 * function in __ct_hashf and calls __ct_hash, compiled in the library's
 * calling convention (ln45 bridges the assembly's call). A hash function
 * must not use a hash table itself.
 */
unsigned int (*__ct_hashf)(const void *);

unsigned int __ct_hash(const void *k) {
    return __ct_hashf(k);
}
