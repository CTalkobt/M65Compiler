/* map.h — ordered maps (#407): struct map (a sorted array) and struct tmap
 * (a red-black tree), <set.h>'s sets whose element is a key and then a value.
 *
 *   static int by_id(const void *a, const void *b) { ... }   (sees the keys)
 *   struct map m;
 *   m.init(sizeof(int), sizeof(struct rec), by_id);
 *   m.put(&id, &r);                1 inserted, 0 replaced, -1 no memory
 *   struct rec *p = m.get(&id);    the value, or NULL
 *   for (e = m.first(); e; e = m.next(e)) ...   in key order: e the key, the
 *                                               value ksize bytes after it
 *   m.erase(&id);
 *   m.free();
 *
 * cmp(a, b) compares two keys (< 0: a first; 0: the same key); NULL compares
 * the key's bytes, unsigned, the first first (a big-endian key or a string).
 * The key and the value are copied in by their sizes. cmp is called through
 * the library's shim, as qsort's: it must not use a set or a map itself.
 * Not reentrant (not from an __interrupt handler while the program uses one).
 *
 * struct map: one block, a binary search, put and erase moving the elements
 * after the place: small, for tens to a few hundred keys; element (and value)
 * addresses change on put and erase. struct tmap: a tree of nodes, O(log n);
 * an element stays where it is until it is erased. The code is the sets'
 * (stdlib_asm/set.s45, tset.s45).
 */
#ifndef _MAP_H
#define _MAP_H

#include <set.h>

struct map {
    unsigned char *data;
    size_t len;         /* elements */
    size_t cap;
    size_t esize;       /* ksize + vsize */
    set_cmp_t cmp;
    size_t ksize;

    __fastcall__ void init(size_t ksize, size_t vsize, set_cmp_t cmp);
    /* 1 inserted, 0 the value replaced, -1 no memory */
    __fastcall__ int put(const void *key, const void *val);
    /* the value, or NULL */
    __fastcall__ void *get(const void *key);
    /* 1 erased, 0 not there */
    __fastcall__ int erase(const void *key);
    /* the element (its key; the value ksize bytes on), or NULL */
    __fastcall__ void *find(const void *key);
    /* the first element whose key is not before key, or NULL */
    __fastcall__ void *lower_bound(const void *key);
    /* in key order: the first, the one after e; NULL at the end */
    __fastcall__ void *first();
    __fastcall__ void *next(void *e);
    __fastcall__ size_t size();
    /* empty; clear keeps the memory, free gives it back */
    __fastcall__ void clear();
    __fastcall__ void free();
};
typedef struct map map_t;

struct tmap {
    void *root;
    size_t len;
    size_t esize;
    set_cmp_t cmp;
    size_t ksize;

    __fastcall__ void init(size_t ksize, size_t vsize, set_cmp_t cmp);
    __fastcall__ int put(const void *key, const void *val);
    __fastcall__ void *get(const void *key);
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
typedef struct tmap tmap_t;

#endif
