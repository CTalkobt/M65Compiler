/* hashmap.h — hash tables (#407): struct hashmap (a key and a value) and
 * struct hashset (keys), open addressing with linear probing.
 *
 *   struct hashmap m;
 *   m.init(sizeof(int), sizeof(struct rec), 0, NULL, NULL);
 *   m.put(&id, &r);                1 inserted, 0 replaced, -1 no memory
 *   struct rec *p = m.get(&id);    the value, or NULL
 *   m.erase(&id);
 *   for (e = m.first(); e; e = m.next(e)) ...   every element, in no order
 *   m.free();
 *
 *   struct hashmap names;          C string keys: ksize 0
 *   names.init(0, sizeof(int), 64, NULL, NULL);
 *   names.put("player", &n);       the table keeps the pointer, not a copy
 *
 * Keys are ksize bytes copied in (or, ksize 0, C strings the table points
 * to: they must stay as they are while in it); values vsize bytes copied in.
 * hash NULL: hash_bytes over the key (hash_str for strings); cmp NULL: the
 * key's bytes compared (strings: equal by strcmp). A program's cmp returns 0
 * when the keys are equal (memcmp, strcmp); its hash must give equal keys
 * equal hashes. Both are called through the library's shims, as qsort's
 * comparison: they must not use a hash table. Not reentrant (not from an
 * __interrupt handler while the program uses one).
 *
 * cap: the slots to start with (a power of two, at least 8; allocated by the
 * first put). A put that would fill over 3/4 of the slots (erased ones
 * count) rebuilds the table for twice the elements: element addresses
 * change on put; erase leaves them. An element is the key (for strings, the
 * pointer: *(char **)e), then the value. Assembly: stdlib_asm/hashmap.s45.
 */
#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <stddef.h>

typedef unsigned int (*hash_fn_t)(const void *);
typedef int (*hash_cmp_t)(const void *, const void *);

/* the built-in hashes (djb2: h * 33 ^ byte) */
__fastcall__ unsigned int hash_bytes(const void *p, size_t n);
__fastcall__ unsigned int hash_str(const char *s);

struct hashmap {
    unsigned char *slots;
    size_t cap;
    size_t len;         /* elements */
    size_t used;        /* elements and erased slots */
    size_t ksize;       /* 0: C strings */
    size_t vsize;
    hash_fn_t hash;
    hash_cmp_t cmp;

    __fastcall__ void init(size_t ksize, size_t vsize, size_t cap, hash_fn_t hash, hash_cmp_t cmp);
    /* 1 inserted, 0 the value replaced, -1 no memory */
    __fastcall__ int put(const void *key, const void *val);
    /* the value, or NULL */
    __fastcall__ void *get(const void *key);
    /* the element (its key, the value after it), or NULL */
    __fastcall__ void *find(const void *key);
    /* 1 erased, 0 not there */
    __fastcall__ int erase(const void *key);
    /* every element, in no order: the first, the one after e; NULL at the end */
    __fastcall__ void *first();
    __fastcall__ void *next(void *e);
    __fastcall__ size_t size();
    /* rebuilt with n slots at least (and room for the elements): 0, -1 no memory */
    __fastcall__ int rehash(size_t n);
    /* empty; clear keeps the memory, free gives it back */
    __fastcall__ void clear();
    __fastcall__ void free();
};
typedef struct hashmap hashmap_t;

struct hashset {
    unsigned char *slots;
    size_t cap;
    size_t len;
    size_t used;
    size_t ksize;
    size_t vsize;       /* 0 */
    hash_fn_t hash;
    hash_cmp_t cmp;

    __fastcall__ void init(size_t ksize, size_t cap, hash_fn_t hash, hash_cmp_t cmp);
    /* 1 inserted, 0 there already, -1 no memory */
    __fastcall__ int insert(const void *key);
    __fastcall__ void *find(const void *key);
    __fastcall__ int erase(const void *key);
    __fastcall__ void *first();
    __fastcall__ void *next(void *e);
    __fastcall__ size_t size();
    __fastcall__ int rehash(size_t n);
    __fastcall__ void clear();
    __fastcall__ void free();
};
typedef struct hashset hashset_t;

#endif
