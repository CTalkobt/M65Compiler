/* test_pqueue_xemu.c — <pqueue.h> (#405): a binary heap
 *
 * Results at $C0C0 (01 = pass):
 *   [0] ints with a comparator: random pushes, popped in order; full / empty
 *   [1] cmp NULL: 2-byte keys stored high byte first, compared by their bytes
 *   [2] 5-byte structs (the multiplier) by a field, the largest first (cmp reversed);
 *       equal keys all come out
 *   [3] pushes and pops mixed against a sorted array model; top, count, NULL out
 *   [4] 1-byte elements, cap 1; pop on empty, push on full
 *   [5] $AA sentinel
 */
#include <string.h>
#include <pqueue.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int seed = 777;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 3; }

static int by_int(const void *a, const void *b) {
    signed int x = *(const signed int *)a, y = *(const signed int *)b;
    return x < y ? -1 : x > y;
}

struct job { unsigned char id; signed int prio; unsigned int due; };

static int by_prio_desc(const void *a, const void *b) {
    signed int x = ((const struct job *)a)->prio, y = ((const struct job *)b)->prio;
    return x > y ? -1 : x < y;
}

static int t_ints(void) {
    static signed int buf[40];
    pqueue_t q;
    signed int v, last = -32768;
    unsigned int i;
    int ok = 1;
    pq_init(&q, buf, sizeof(int), 40, by_int);
    if (pq_top(&q) != NULL || pq_pop(&q, &v) != -1) ok = 0;
    for (i = 0; i < 40; i++) { v = (signed int)(rnd() % 2000) - 1000; if (pq_push(&q, &v) != 0) ok = 0; }
    if (pq_push(&q, &v) != -1 || pq_count(&q) != 40) ok = 0;
    for (i = 0; i < 40; i++) {
        signed int top = *(signed int *)pq_top(&q);
        if (pq_pop(&q, &v) != 0 || v != top || v < last) ok = 0;
        last = v;
    }
    return ok && pq_count(&q) == 0;
}

static int t_bytes(void) {
    static unsigned char buf[30][2];
    pqueue_t q;
    unsigned char k[2];
    unsigned int i, v, last = 0;
    int ok = 1;
    pq_init(&q, buf, 2, 30, NULL);
    for (i = 0; i < 30; i++) {
        v = rnd();
        k[0] = (unsigned char)(v >> 8); k[1] = (unsigned char)v;     /* big-endian: by bytes is by value */
        pq_push(&q, k);
    }
    for (i = 0; i < 30; i++) {
        pq_pop(&q, k);
        v = (k[0] << 8) | k[1];
        if (v < last) ok = 0;
        last = v;
    }
    return ok;
}

static int t_structs(void) {
    static struct job buf[25];
    pqueue_t q;
    struct job j;
    signed int last = 32767;
    unsigned int i, seen = 0;
    int ok = 1;
    pq_init(&q, buf, sizeof(struct job), 25, by_prio_desc);
    for (i = 0; i < 25; i++) {
        j.id = (unsigned char)i; j.prio = (signed int)(i % 5) * 10 - 20; j.due = i * 3;
        pq_push(&q, &j);
    }
    for (i = 0; i < 25; i++) {
        pq_pop(&q, &j);
        if (j.prio > last || j.due != j.id * 3u) ok = 0;
        seen |= 1u << (j.id % 16);
        last = j.prio;
    }
    return ok && sizeof(struct job) == 5;
}

static int t_model(void) {
    static unsigned int buf[20];
    static unsigned int model[20];
    pqueue_t q;
    unsigned int n, v, i, m = 0;
    int ok = 1;
    pq_init(&q, buf, sizeof(unsigned int), 20, by_int);
    for (n = 0; n < 1500 && ok; n++) {
        if ((rnd() & 1) && m < 20) {
            v = rnd() & 0x3FF;
            if (pq_push(&q, &v) != 0) ok = 0;
            for (i = m; i > 0 && model[i - 1] > v; i--) model[i] = model[i - 1];
            model[i] = v;
            m++;
        } else if (m) {
            if (*(unsigned int *)pq_top(&q) != model[0]) ok = 0;
            if (n & 4) { if (pq_pop(&q, NULL) != 0) ok = 0; }
            else { if (pq_pop(&q, &v) != 0 || v != model[0]) ok = 0; }
            memmove(model, model + 1, (m - 1) * sizeof model[0]);
            m--;
        }
        if (pq_count(&q) != m) ok = 0;
    }
    return ok;
}

static int t_one(void) {
    static unsigned char b[1];
    pqueue_t q;
    unsigned char c = 5, o = 0;
    int ok = 1;
    pq_init(&q, b, 1, 1, NULL);
    if (pq_pop(&q, &o) != -1) ok = 0;
    if (pq_push(&q, &c) != 0 || pq_push(&q, &c) != -1) ok = 0;
    if (pq_pop(&q, &o) != 0 || o != 5 || pq_count(&q) != 0) ok = 0;
    return ok;
}

int main(void) {
    OUT[0] = t_ints();
    OUT[1] = t_bytes();
    OUT[2] = t_structs();
    OUT[3] = t_model();
    OUT[4] = t_one();
    OUT[5] = 0xAA;
    return 0;
}
