/* vector.h — a growable array, and views of arrays and strings (#404)
 *
 *   vec_t v;
 *   vec_init(&v, sizeof(struct enemy));
 *   struct enemy *e = vec_push(&v, &proto);   the new element, NULL without memory
 *   for (i = 0; i < v.size; i++) ... vec_at(&v, i) ...
 *   vec_erase_unordered(&v, i);               the last element into the hole
 *   vec_free(&v);
 *
 * The vector is assembly (stdlib_asm/vector.s45): type-erased, elements copied
 * in and out by their size. It grows by doubling its capacity (4 at first)
 * with realloc: element addresses change when it grows, so do not keep a
 * pointer across a push or insert. cap * esize stays below 64 KB. Not
 * reentrant (not from an __interrupt handler while the program uses one).
 *
 * The views (span_t, strview_t) are a pointer and a length: static inline
 * here, nothing in the library.
 */
#ifndef _VECTOR_H
#define _VECTOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct vec {
    unsigned char *data;
    size_t size;        /* elements in it */
    size_t cap;         /* elements there is room for */
    size_t esize;
} vec_t;

__fastcall__ void vec_init(vec_t *v, size_t esize);
/* room for n elements: 0, or -1 without memory */
__fastcall__ int vec_reserve(vec_t *v, size_t n);
/* the capacity down to the size (no memory when empty) */
__fastcall__ void vec_shrink(vec_t *v);
/* empty, the memory kept */
__fastcall__ void vec_clear(vec_t *v);
/* empty, the memory freed */
__fastcall__ void vec_free(vec_t *v);
/* e copied to the end: its place, or NULL without memory (e NULL: the place, not filled) */
__fastcall__ void *vec_push(vec_t *v, const void *e);
/* the last element into out (out may be NULL): 0, or -1 when empty */
__fastcall__ int vec_pop(vec_t *v, void *out);
/* element i, the last: NULL when there is none */
__fastcall__ void *vec_at(vec_t *v, size_t i);
__fastcall__ void *vec_back(vec_t *v);
/* e copied in before element i (i <= size): its place, or NULL */
__fastcall__ void *vec_insert(vec_t *v, size_t i, const void *e);
/* element i out, the rest moved down: 0, or -1 past the end */
__fastcall__ int vec_erase(vec_t *v, size_t i);
/* element i out, the last moved into its place (the order not kept): 0, or -1 */
__fastcall__ int vec_erase_unordered(vec_t *v, size_t i);

#define vec_size(v)   ((v)->size)
#define vec_empty(v)  ((v)->size == 0)
#define vec_data(v)   ((void *)(v)->data)
/* sorted, searched: qsort / bsearch on the elements */
#define vec_sort(v, cmp)         qsort((v)->data, (v)->size, (v)->esize, (cmp))
#define vec_bsearch(v, key, cmp) bsearch((key), (v)->data, (v)->size, (v)->esize, (cmp))

/* ---- views: a pointer and a length, the memory someone else's ---- */

typedef struct span {
    void *ptr;
    size_t len;         /* elements */
    size_t esize;
} span_t;

static inline span_t span_of(void *p, size_t len, size_t esize) {
    span_t s;
    s.ptr = p; s.len = len; s.esize = esize;
    return s;
}
static inline span_t span_of_vec(vec_t *v) { return span_of(v->data, v->size, v->esize); }
/* element i (no check) */
#define span_at(s, i) ((void *)((unsigned char *)(s).ptr + (size_t)(i) * (s).esize))
/* elements from..from+n-1, cut to the span */
static inline span_t span_sub(span_t s, size_t from, size_t n) {
    if (from > s.len) from = s.len;
    if (n > s.len - from) n = s.len - from;
    return span_of((unsigned char *)s.ptr + from * s.esize, n, s.esize);
}

typedef struct strview {
    const char *p;
    size_t len;
} strview_t;

static inline strview_t sv_make(const char *p, size_t len) {
    strview_t v;
    v.p = p; v.len = len;
    return v;
}
/* a C string's view (its length by strlen) */
static inline strview_t sv_from(const char *s) { return sv_make(s, strlen(s)); }
static inline int sv_eq(strview_t a, strview_t b) {
    return a.len == b.len && (a.len == 0 || memcmp(a.p, b.p, a.len) == 0);
}
static inline int sv_starts_with(strview_t s, strview_t prefix) {
    return prefix.len <= s.len && (prefix.len == 0 || memcmp(s.p, prefix.p, prefix.len) == 0);
}
/* the first ch's index, or -1 */
static inline signed int sv_find(strview_t s, char ch) {
    const char *f;
    if (s.len == 0) return -1;
    f = memchr(s.p, ch, s.len);
    return f ? (signed int)(f - s.p) : -1;
}
/* characters pos..pos+n-1, cut to the view */
static inline strview_t sv_sub(strview_t s, size_t pos, size_t n) {
    if (pos > s.len) pos = s.len;
    if (n > s.len - pos) n = s.len - pos;
    return sv_make(s.p + pos, n);
}

#endif
