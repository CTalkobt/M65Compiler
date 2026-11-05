/* set.h — ordered sets (#403): struct set (a sorted array) and struct tset
 * (a red-black tree), with the same methods.
 *
 *   static int by_id(const void *a, const void *b) { ... }
 *   struct set s;
 *   s.init(sizeof(struct rec), by_id);
 *   s.insert(&r);                  1 inserted, 0 there already, -1 no memory
 *   if (s.find(&key)) ...
 *   for (p = s.first(); p; p = s.next(p)) ...      in order
 *   s.erase(&key);
 *   s.free();
 *
 * Type-erased, as qsort: a set keeps its element size and a comparator,
 * cmp(a, b) < 0 when a comes before b (0: the same element); cmp NULL
 * compares the elements' bytes, unsigned, the first byte first (a big-endian
 * key or a string). Elements are copied in by their size. cmp is called
 * through the library's shim (stdlib/ct_shim.c), as qsort's: it must not use
 * a set itself. Not reentrant (not from an __interrupt handler while the
 * program uses a set).
 *
 * struct set: one block (realloc), a binary search, insert and erase moving
 * the elements after it — small; for tens to a few hundred elements. Element
 * addresses change on insert and erase. (Its first fields are a vec_t's: the
 * array is <vector.h>'s.) Assembly: stdlib_asm/set.s45.
 *
 * struct tset: a red-black tree of nodes (malloc), O(log n) insert and erase;
 * an element stays where it is until it is erased. Assembly: stdlib_asm/tset.s45.
 *
 * Maps on them (a key and a value): <map.h>.
 */
#ifndef _SET_H
#define _SET_H

#include <stddef.h>

typedef int (*set_cmp_t)(const void *, const void *);

struct set {
    unsigned char *data;
    size_t len;         /* elements */
    size_t cap;
    size_t esize;
    set_cmp_t cmp;
    size_t ksize;       /* the bytes cmp NULL compares: esize (a map's key) */

    __fastcall__ void init(size_t esize, set_cmp_t cmp);
    /* 1 inserted, 0 there already, -1 no memory */
    __fastcall__ int insert(const void *key);
    /* 1 erased, 0 not there */
    __fastcall__ int erase(const void *key);
    /* the element equal to key, or NULL */
    __fastcall__ void *find(const void *key);
    /* the first element not before key, or NULL */
    __fastcall__ void *lower_bound(const void *key);
    /* in order: the first, the one after e; NULL at the end */
    __fastcall__ void *first();
    __fastcall__ void *next(void *e);
    __fastcall__ size_t size();
    /* empty; clear keeps the memory, free gives it back */
    __fastcall__ void clear();
    __fastcall__ void free();
};
typedef struct set set_t;

struct tset {
    void *root;         /* the root node (tset.s45's layout) */
    size_t len;
    size_t esize;
    set_cmp_t cmp;
    size_t ksize;

    __fastcall__ void init(size_t esize, set_cmp_t cmp);
    __fastcall__ int insert(const void *key);
    __fastcall__ int erase(const void *key);
    __fastcall__ void *find(const void *key);
    __fastcall__ void *lower_bound(const void *key);
    __fastcall__ void *first();
    __fastcall__ void *next(void *e);
    __fastcall__ size_t size();
    /* empty: every node freed */
    __fastcall__ void clear();
    __fastcall__ void free();
};
typedef struct tset tset_t;

#endif
