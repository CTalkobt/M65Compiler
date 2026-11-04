/* test_ringbuf_xemu.c — <ringbuf.h> (#405): the deque against a reference,
 * the byte queue, and one producer in a raster interrupt with the program as
 * the consumer
 *
 * Results at $C0C0 (01 = pass):
 *   [0] ints, cap 4: full after 4, first in first out, count / full / empty
 *   [1] 3-byte elements, cap 5 (the multiplier; any cap): 2000 random
 *       push_back / push_front / pop_front / pop_back against an array model,
 *       front / back / at / count after each
 *   [2] bytes, cap 7, the same model (1-byte elements); a stack and a queue by the macros
 *   [3] the byte queue: 255 then full, the order, peek, count, empty again
 *   [4] a raster interrupt putting bytes (rbb) and words (rb, cap 64) while the
 *       program takes them: 3000 of each, in order, none lost, the queues full
 *       now and then
 *   [5] rb_clear, NULL out, rb_at past the end, a 300-byte element (a page and more)
 *   [6] $AA sentinel
 */
#include <string.h>
#include <ringbuf.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned int seed = 12345;
static unsigned int rnd(void) { seed = seed * 25173u + 13849u; return seed >> 4; }

/* ---- [0] ---- */
static int t_ints(void) {
    static int buf[4];
    ringbuf_t r;
    int v, ok = 1, i;
    rb_init(&r, buf, sizeof(int), 4);
    if (!rb_empty(&r) || rb_full(&r) || rb_count(&r) != 0) ok = 0;
    for (i = 0; i < 4; i++) { v = 100 + i; if (rb_push_back(&r, &v) != 0) ok = 0; }
    v = 99;
    if (rb_push_back(&r, &v) != -1 || !rb_full(&r) || rb_count(&r) != 4) ok = 0;
    for (i = 0; i < 4; i++) { if (rb_pop_front(&r, &v) != 0 || v != 100 + i) ok = 0; }
    if (rb_pop_front(&r, &v) != -1 || !rb_empty(&r)) ok = 0;
    return ok;
}

/* ---- [1], [2]: random operations against an array model ---- */
static unsigned char model[16][3];
static int mcount;

static int against_model(unsigned int esize, unsigned int cap, unsigned int rounds) {
    static unsigned char store[16 * 3];
    unsigned char e[3], o[3];
    ringbuf_t r;
    unsigned int n, k;
    int ok = 1;
    rb_init(&r, store, esize, cap);
    mcount = 0;
    for (n = 0; n < rounds && ok; n++) {
        unsigned int op = rnd() & 3;
        e[0] = (unsigned char)n; e[1] = (unsigned char)(n >> 8); e[2] = (unsigned char)(n * 7);
        if (op == 0) {
            signed int got = rb_push_back(&r, e);
            if (mcount == (int)cap) { if (got != -1) ok = 0; }
            else { if (got != 0) ok = 0; memcpy(model[mcount++], e, esize); }
        } else if (op == 1) {
            signed int got = rb_push_front(&r, e);
            if (mcount == (int)cap) { if (got != -1) ok = 0; }
            else {
                if (got != 0) ok = 0;
                memmove(model[1], model[0], sizeof model[0] * mcount);
                memcpy(model[0], e, esize);
                mcount++;
            }
        } else if (op == 2) {
            signed int got = rb_pop_front(&r, o);
            if (mcount == 0) { if (got != -1) ok = 0; }
            else {
                if (got != 0 || memcmp(o, model[0], esize) != 0) ok = 0;
                memmove(model[0], model[1], sizeof model[0] * (mcount - 1));
                mcount--;
            }
        } else {
            signed int got = rb_pop_back(&r, o);
            if (mcount == 0) { if (got != -1) ok = 0; }
            else {
                mcount--;
                if (got != 0 || memcmp(o, model[mcount], esize) != 0) ok = 0;
            }
        }
        if ((int)rb_count(&r) != mcount || rb_empty(&r) != (mcount == 0) || rb_full(&r) != (mcount == (int)cap)) ok = 0;
        if (mcount) {
            if (memcmp(rb_front(&r), model[0], esize) != 0 || memcmp(rb_back(&r), model[mcount - 1], esize) != 0) ok = 0;
            for (k = 0; k < (unsigned int)mcount; k++)
                if (memcmp(rb_at(&r, k), model[k], esize) != 0) ok = 0;
        } else if (rb_front(&r) || rb_back(&r)) ok = 0;
        if (rb_at(&r, mcount) != NULL) ok = 0;
    }
    return ok;
}

static int t_macros(void) {
    static unsigned char b[3];
    ringbuf_t s;
    unsigned char c;
    int ok = 1;
    rb_init(&s, b, 1, 3);
    c = 1; rb_push(&s, &c); c = 2; rb_push(&s, &c); c = 3; rb_push(&s, &c);
    if (*(unsigned char *)rb_top(&s) != 3) ok = 0;
    rb_pop(&s, &c); if (c != 3) ok = 0;
    rb_pop(&s, &c); if (c != 2) ok = 0;
    c = 9; rb_enqueue(&s, &c);
    rb_dequeue(&s, &c); if (c != 1) ok = 0;
    rb_dequeue(&s, &c); if (c != 9) ok = 0;
    return ok && rb_empty(&s);
}

/* ---- [3] ---- */
static rbb_t bq;
static int t_bytes(void) {
    unsigned int i;
    int ok = 1;
    rbb_init(&bq);
    if (!rbb_empty(&bq) || rbb_get(&bq) != -1 || rbb_peek(&bq) != -1) ok = 0;
    for (i = 0; i < 255; i++) if (rbb_put(&bq, (unsigned char)(i * 3)) != 0) ok = 0;
    if (rbb_put(&bq, 1) != -1 || !rbb_full(&bq) || rbb_count(&bq) != 255) ok = 0;
    if (rbb_peek(&bq) != 0) ok = 0;
    for (i = 0; i < 255; i++) if (rbb_get(&bq) != (int)(unsigned char)(i * 3)) ok = 0;
    if (!rbb_empty(&bq) || rbb_count(&bq) != 0) ok = 0;
    for (i = 0; i < 600; i++) {    /* round and round */
        rbb_put(&bq, (unsigned char)i);
        if (rbb_get(&bq) != (int)(unsigned char)i) ok = 0;
    }
    return ok;
}

/* ---- [4] the interrupt producer ---- */
static rbb_t iq;
static ringbuf_t wq;
static unsigned int wbuf[64];
static volatile unsigned char bnext;
static volatile unsigned int wnext;
static volatile unsigned int irqs, fulls;
static unsigned char old_vector[2], old_d01a, old_d012;

__interrupt void producer(void) {
    unsigned char n;
    for (n = 0; n < 4; n++) {      /* a few each time: the queues fill now and then */
        unsigned int w = wnext;
        if (rbb_put(&iq, bnext) == 0) bnext++;
        else fulls++;
        if (rb_push_back(&wq, &w) == 0) wnext = w + 1;
        else fulls++;
    }
    irqs++;
    REG(0xD012) = REG(0xD012) + 9;     /* the next a few lines on */
    REG(0xD019) = 0x01;
}

static void irq_install(void) {
    static const unsigned char stub[] = {
        0xAD, 0x19, 0xD0, 0x29, 0x01, 0xD0, 0x03, 0x6C, 0x00, 0x00,
        0xA9, 0x00, 0x48, 0xA9, 0x00, 0x48, 0x08, 0x4C, 0x00, 0x00,
        0x68, 0x5B, 0xFB, 0x7A, 0xFA, 0x68, 0x40,
    };
    unsigned char *st = (unsigned char *)0xBE00;
    unsigned int tail = 0xBE00 + 20, isr = (unsigned int)producer;
    memcpy(st, stub, sizeof stub);
    st[27] = REG(0x0314); st[28] = REG(0x0315);
    st[8] = 27; st[9] = 0xBE;
    st[11] = (unsigned char)(tail >> 8); st[14] = (unsigned char)tail;
    st[18] = (unsigned char)isr; st[19] = (unsigned char)(isr >> 8);
    __asm__("sei");
    old_vector[0] = REG(0x0314); old_vector[1] = REG(0x0315);
    old_d01a = REG(0xD01A); old_d012 = REG(0xD012);
    REG(0xD05D) = REG(0xD05D) & 0x7F;
    REG(0xD011) = REG(0xD011) & 0x7F;
    REG(0xD012) = 0x10;
    REG(0x0314) = 0x00; REG(0x0315) = 0xBE;
    REG(0xD01A) = 0x01;
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static void irq_remove(void) {
    __asm__("sei");
    REG(0x0314) = old_vector[0]; REG(0x0315) = old_vector[1];
    REG(0xD012) = old_d012;
    REG(0xD01A) = old_d01a & 0x0F;
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static int t_interrupt(void) {
    unsigned int gotb = 0, gotw = 0, w, spin;
    unsigned char wantb = 0;
    unsigned int wantw = 0;
    unsigned long guard = 0;
    int ok = 1;
    rbb_init(&iq);
    rb_init(&wq, wbuf, sizeof(unsigned int), 64);
    bnext = 0; wnext = 0; irqs = 0; fulls = 0;
    irq_install();
    while ((gotb < 3000 || gotw < 3000) && ok && ++guard < 2000000UL) {
        signed int c = rbb_get(&iq);
        if (c >= 0) {
            if (c != wantb) ok = 0;
            wantb++;
            gotb++;
        }
        if (rb_pop_front(&wq, &w) == 0) {
            if (w != wantw) ok = 0;
            wantw++;
            gotw++;
        }
        for (spin = (rnd() & 255) == 0 ? 20000 : rnd() & 63; spin; spin--) ;   /* now and then let them fill */
    }
    irq_remove();
    return ok && gotb >= 3000 && gotw >= 3000 && irqs > 0 && fulls > 0;
}

/* ---- [5] ---- */
static int t_edges(void) {
    static unsigned char big[3][300];
    static unsigned char e[300], o[300];
    ringbuf_t r;
    unsigned int i;
    int ok = 1;
    rb_init(&r, big, 300, 3);
    for (i = 0; i < 300; i++) e[i] = (unsigned char)(i ^ 0x5A);
    if (rb_push_back(&r, e) != 0 || rb_push_back(&r, e) != 0) ok = 0;
    if (rb_pop_front(&r, NULL) != 0 || rb_count(&r) != 1) ok = 0;
    e[299] = 7;
    if (rb_push_front(&r, e) != 0) ok = 0;
    if (rb_pop_front(&r, o) != 0 || memcmp(o, e, 300) != 0) ok = 0;
    if (rb_at(&r, 1) != NULL || rb_at(&r, 0) == NULL) ok = 0;
    rb_clear(&r);
    if (!rb_empty(&r) || rb_pop_back(&r, o) != -1 || rb_front(&r) != NULL) ok = 0;
    return ok;
}

int main(void) {
    OUT[0] = t_ints();
    OUT[1] = against_model(3, 5, 2000);
    OUT[2] = against_model(1, 7, 2000) && t_macros();
    OUT[3] = t_bytes();
    OUT[4] = t_interrupt();
    OUT[5] = t_edges();
    OUT[6] = 0xAA;
    return 0;
}
