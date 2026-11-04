/* test_vec_xemu.c — <vector.h> (#404): the vector against an array model,
 * and the views
 *
 * Results at $C0C0 (01 = pass):
 *   [0] ints: push 100 (growing from 4), at / back / size, pop in reverse
 *   [1] 3-byte elements: 1500 random push / pop / insert / erase /
 *       erase_unordered against an array model, every element checked
 *   [2] reserve, shrink, clear, free; vec_push(NULL) makes a place
 *   [3] vec_sort and vec_bsearch on 1-byte and int elements
 *   [4] out of memory: a push that cannot grow gives NULL, the vector intact
 *   [5] views: span_sub, span_at; sv_from, sv_eq, sv_starts_with, sv_find, sv_sub
 *   [6] $AA sentinel
 */
#include <string.h>
#include <stdlib.h>
#include <vector.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 4242;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

static int t_ints(void) {
    vec_t v;
    signed int i, x;
    int ok = 1;
    vec_init(&v, sizeof(int));
    if (vec_back(&v) != NULL || vec_pop(&v, &x) != -1 || vec_at(&v, 0) != NULL) ok = 0;
    for (i = 0; i < 100; i++) {
        x = i * 7 - 300;
        if (vec_push(&v, &x) == NULL) ok = 0;
    }
    if (v.size != 100 || v.cap < 100 || *(signed int *)vec_back(&v) != 99 * 7 - 300) ok = 0;
    for (i = 0; i < 100; i++) if (*(signed int *)vec_at(&v, i) != i * 7 - 300) ok = 0;
    if (vec_at(&v, 100) != NULL) ok = 0;
    for (i = 99; i >= 0; i--) { if (vec_pop(&v, &x) != 0 || x != i * 7 - 300) ok = 0; }
    if (!vec_empty(&v)) ok = 0;
    vec_free(&v);
    return ok && v.data == NULL && v.cap == 0;
}

static unsigned char model[64][3];
static unsigned int msize;

static int t_model(void) {
    vec_t v;
    unsigned char e[3], o[3];
    unsigned int n, k, i;
    int ok = 1;
    vec_init(&v, 3);
    msize = 0;
    for (n = 0; n < 1500 && ok; n++) {
        unsigned int op = rnd() % 5;
        e[0] = (unsigned char)n; e[1] = (unsigned char)(n >> 8); e[2] = (unsigned char)(n * 13);
        if (op == 0 && msize < 64) {
            unsigned char *p = vec_push(&v, e);
            if (!p || memcmp(p, e, 3) != 0) ok = 0;
            memcpy(model[msize++], e, 3);
        } else if (op == 1) {
            signed int got = vec_pop(&v, o);
            if (msize == 0) { if (got != -1) ok = 0; }
            else { msize--; if (got != 0 || memcmp(o, model[msize], 3) != 0) ok = 0; }
        } else if (op == 2 && msize < 64) {
            i = msize ? rnd() % (msize + 1) : 0;
            if (vec_insert(&v, i, e) == NULL) ok = 0;
            memmove(model[i + 1], model[i], 3 * (msize - i));
            memcpy(model[i], e, 3);
            msize++;
            if (vec_insert(&v, msize + 1, e) != NULL) ok = 0;   /* past the end */
        } else if (op == 3) {
            i = msize ? rnd() % msize : 0;
            if (msize == 0) { if (vec_erase(&v, 0) != -1) ok = 0; }
            else {
                if (vec_erase(&v, i) != 0) ok = 0;
                memmove(model[i], model[i + 1], 3 * (msize - i - 1));
                msize--;
            }
        } else if (op == 4) {
            i = msize ? rnd() % msize : 0;
            if (msize == 0) { if (vec_erase_unordered(&v, 0) != -1) ok = 0; }
            else {
                if (vec_erase_unordered(&v, i) != 0) ok = 0;
                msize--;
                if (i != msize) memcpy(model[i], model[msize], 3);
            }
        }
        if (v.size != msize) ok = 0;
        for (k = 0; k < msize; k++) if (memcmp(vec_at(&v, k), model[k], 3) != 0) ok = 0;
    }
    vec_free(&v);
    return ok;
}

static int t_capacity(void) {
    vec_t v;
    int ok = 1;
    unsigned char *p;
    vec_init(&v, 5);
    if (vec_reserve(&v, 50) != 0 || v.cap != 50 || v.data == NULL) ok = 0;
    if (vec_reserve(&v, 10) != 0 || v.cap != 50) ok = 0;   /* already room */
    p = vec_push(&v, NULL);
    if (p == NULL || p != v.data || v.size != 1) ok = 0;
    memset(p, 0x5A, 5);
    vec_push(&v, p);
    vec_shrink(&v);
    if (v.cap != 2 || v.size != 2 || ((unsigned char *)vec_at(&v, 1))[4] != 0x5A) ok = 0;
    vec_clear(&v);
    if (v.size != 0 || v.cap != 2) ok = 0;
    vec_shrink(&v);
    if (v.cap != 0 || v.data != NULL) ok = 0;
    vec_free(&v);
    return ok;
}

static int by_byte(const void *a, const void *b) { return (signed int)*(const unsigned char *)a - *(const unsigned char *)b; }
static int by_int(const void *a, const void *b) {
    signed int x = *(const signed int *)a, y = *(const signed int *)b;
    return x < y ? -1 : x > y;
}

static int t_sort(void) {
    vec_t b, w;
    unsigned char c;
    signed int x, *f;
    unsigned int i;
    int ok = 1;
    vec_init(&b, 1);
    vec_init(&w, sizeof(int));
    for (i = 0; i < 60; i++) {
        c = (unsigned char)rnd(); vec_push(&b, &c);
        x = (signed int)(rnd() % 1000) - 500; vec_push(&w, &x);
    }
    x = 1234; vec_push(&w, &x);
    vec_sort(&b, by_byte);
    vec_sort(&w, by_int);
    for (i = 1; i < 60; i++) {
        if (*(unsigned char *)vec_at(&b, i - 1) > *(unsigned char *)vec_at(&b, i)) ok = 0;
        if (*(signed int *)vec_at(&w, i - 1) > *(signed int *)vec_at(&w, i)) ok = 0;
    }
    x = 1234;
    f = vec_bsearch(&w, &x, by_int);
    if (f == NULL || *f != 1234 || f != vec_back(&w)) ok = 0;
    x = 9999;
    if (vec_bsearch(&w, &x, by_int) != NULL) ok = 0;
    vec_free(&b);
    vec_free(&w);
    return ok;
}

static int t_nomem(void) {
    vec_t v;
    unsigned int n = 0;
    int ok = 1;
    void *p;
    vec_init(&v, 1000);
    while ((p = vec_push(&v, NULL)) != NULL && n < 200) n++;
    /* the heap (about 20 KB here) runs out before 200 KB */
    if (p != NULL || n == 0 || v.size != n) ok = 0;
    if (vec_at(&v, n - 1) == NULL) ok = 0;
    vec_free(&v);
    return ok;
}

static int t_views(void) {
    static int a[6] = {10, 20, 30, 40, 50, 60};
    span_t s = span_of(a, 6, sizeof(int)), t;
    strview_t hello = sv_from("hello, world"), w;
    int ok = 1;
    t = span_sub(s, 2, 3);
    if (t.len != 3 || *(int *)span_at(t, 0) != 30 || *(int *)span_at(t, 2) != 50) ok = 0;
    t = span_sub(s, 5, 9);
    if (t.len != 1 || *(int *)span_at(t, 0) != 60) ok = 0;
    if (span_sub(s, 9, 1).len != 0) ok = 0;
    if (hello.len != 12) ok = 0;
    if (!sv_starts_with(hello, sv_from("hello")) || sv_starts_with(hello, sv_from("world"))) ok = 0;
    if (sv_find(hello, ',') != 5 || sv_find(hello, 'z') != -1) ok = 0;
    w = sv_sub(hello, 7, 99);
    if (!sv_eq(w, sv_from("world")) || sv_eq(w, sv_from("worlds"))) ok = 0;
    if (!sv_eq(sv_sub(hello, 12, 3), sv_make("", 0))) ok = 0;
    return ok;
}

int main(void) {
    OUT[0] = t_ints();
    OUT[1] = t_model();
    OUT[2] = t_capacity();
    OUT[3] = t_sort();
    OUT[4] = t_nomem();
    OUT[5] = t_views();
    OUT[6] = 0xAA;
    return 0;
}
