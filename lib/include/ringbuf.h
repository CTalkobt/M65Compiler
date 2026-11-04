/* ringbuf.h — ring buffers (#405): a deque of fixed capacity in storage the
 * caller gives, and a byte queue of 256 bytes. Assembly (stdlib_asm/ringbuf.s45);
 * nothing is allocated.
 *
 *   static int keys[16];
 *   ringbuf_t r;
 *   rb_init(&r, keys, sizeof(int), 16);
 *   rb_push_back(&r, &k);              0, or -1 when full
 *   rb_pop_front(&r, &k);              0, or -1 when empty (out may be NULL)
 *
 * All cap slots are used (cap up to 32767, any value). An element is copied
 * in and out by its size (esize, at least 1).
 *
 * Interrupts: with one producer and one consumer — rb_push_back in an
 * __interrupt handler, rb_pop_front in the program (or the other way round)
 * — no lock is needed when cap <= 128 and esize is a power of two: each side
 * writes its own index (one byte), after copying the element. The other
 * functions, and two producers, are not safe that way. The byte queue
 * (rbb_*) is safe the same way for any use of one producer and one consumer.
 */
#ifndef _RINGBUF_H
#define _RINGBUF_H

#include <stddef.h>

typedef struct ringbuf {
    unsigned char *buf;
    size_t esize;
    size_t cap;
    size_t head;    /* the back:  0 .. 2*cap-1 (written by push_back, pop_back) */
    size_t tail;    /* the front: 0 .. 2*cap-1 (written by pop_front, push_front) */
} ringbuf_t;

__fastcall__ void rb_init(ringbuf_t *r, void *buf, size_t esize, size_t cap);
__fastcall__ int rb_push_back(ringbuf_t *r, const void *e);
__fastcall__ int rb_push_front(ringbuf_t *r, const void *e);
__fastcall__ int rb_pop_front(ringbuf_t *r, void *out);
__fastcall__ int rb_pop_back(ringbuf_t *r, void *out);
/* the first, the last, element i from the front: NULL when there is none */
__fastcall__ void *rb_front(ringbuf_t *r);
__fastcall__ void *rb_back(ringbuf_t *r);
__fastcall__ void *rb_at(ringbuf_t *r, size_t i);
__fastcall__ size_t rb_count(ringbuf_t *r);
__fastcall__ int rb_full(ringbuf_t *r);
__fastcall__ int rb_empty(ringbuf_t *r);
__fastcall__ void rb_clear(ringbuf_t *r);

/* a queue (first in, first out) and a stack (last in, first out) */
#define rb_enqueue(r, e)  rb_push_back((r), (e))
#define rb_dequeue(r, o)  rb_pop_front((r), (o))
#define rb_push(r, e)     rb_push_back((r), (e))
#define rb_pop(r, o)      rb_pop_back((r), (o))
#define rb_top(r)         rb_back(r)

/* a byte queue: 256 bytes of storage, 255 queued at most */
typedef struct rbb {
    unsigned char head;     /* written by rbb_put */
    unsigned char tail;     /* written by rbb_get */
    unsigned char buf[256];
} rbb_t;

__fastcall__ void rbb_init(rbb_t *q);
/* 0, or -1 when full */
__fastcall__ int rbb_put(rbb_t *q, unsigned char c);
/* the byte, or -1 when empty */
__fastcall__ int rbb_get(rbb_t *q);
__fastcall__ int rbb_peek(rbb_t *q);
__fastcall__ unsigned int rbb_count(rbb_t *q);
#define rbb_empty(q) ((q)->head == (q)->tail)
#define rbb_full(q)  ((unsigned char)((q)->head + 1) == (q)->tail)

#endif
