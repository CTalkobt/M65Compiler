/* pqueue.h — a priority queue (#405): a binary heap in storage the caller
 * gives. Assembly (stdlib_asm/pqueue.s45); nothing is allocated.
 *
 *   static struct timer t[32];
 *   pqueue_t q;
 *   pq_init(&q, t, sizeof(struct timer), 32, by_due);
 *   pq_push(&q, &tm);                  0, or -1 when full
 *   pq_pop(&q, &tm);                   the first one; -1 when empty (out may be NULL)
 *
 * The top is the element cmp puts first: cmp(a, b) < 0 when a comes before
 * b (the smallest first; reverse cmp for the largest first). cmp NULL
 * compares the elements' bytes, unsigned, the first byte first. cmp must not
 * use a priority queue itself. Not for an __interrupt handler while the
 * program uses one (the queue's state is static).
 */
#ifndef _PQUEUE_H
#define _PQUEUE_H

#include <stddef.h>

typedef struct pqueue {
    unsigned char *buf;
    size_t esize;
    size_t cap;
    size_t count;
    int (*cmp)(const void *, const void *);
} pqueue_t;

__fastcall__ void pq_init(pqueue_t *q, void *buf, size_t esize, size_t cap,
                          int (*cmp)(const void *, const void *));
__fastcall__ int pq_push(pqueue_t *q, const void *e);
__fastcall__ int pq_pop(pqueue_t *q, void *out);
/* the first element, or NULL when empty */
__fastcall__ void *pq_top(pqueue_t *q);
__fastcall__ size_t pq_count(pqueue_t *q);

#endif
