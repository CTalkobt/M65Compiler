/* test_redundant_ldy_xemu.c — #586: ca45 removes an ldy #0 when Y is already
 * 0, and a load of A / Y a cc45 function's rts does not return (A: a void
 * function; Y: all but a long's). Y must still be loaded where it may not be
 * 0: after a loop that steps it, where paths with different Y meet, after a
 * call; and a long's upper bytes (Y, Z) must still be returned.
 *
 * Results at $C0C0: 1 = pass; [10] $AA.
 */
#include <stdint.h>

uint8_t buf[16], src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
uint8_t * volatile vp;

void xr(uint8_t *d, uint8_t k) { *d ^= k; }
void ad(uint8_t *d) { *d += 3; }

/* Y stepped by a loop, then a store at offset 0 */
void copy_then_mark(uint8_t *d, uint8_t *s, uint8_t n) {
    uint8_t i;
    for (i = 0; i < n; i++) d[i] = s[i];
    *d = 0x99;
}
/* paths with Y 1 and Y 0 meet */
void merge(uint8_t *d, uint8_t c) {
    if (c) d[1] = 0x11;
    *d = 0x22;
}
/* a call between two stores through the same pointer */
uint8_t __attribute__((noinline)) bump(uint8_t *p) { p[3]++; return p[3]; }
void across_call(uint8_t *d) {
    *d = 0x10;
    bump(d);
    *d += 1;
}
/* two reads and a store, the pointer read twice */
uint8_t swap_first(uint8_t *a, uint8_t *b) {
    uint8_t t = *a;
    *a = *b;
    *b = t;
    return t;
}
/* a long's Y and Z are its result */
uint32_t __attribute__((noinline)) wide(uint8_t *p) { return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | 0x1234; }
uint32_t __attribute__((noinline)) wide2(uint8_t *p, uint8_t c) { uint32_t v = 0x00AB0000UL; if (c) v = ((uint32_t)*p << 16); return v + 1; }
/* an int's X, a char's A */
uint16_t __attribute__((noinline)) word(uint8_t *p) { return ((uint16_t)*p << 8) | 0x34; }
uint8_t __attribute__((noinline)) byte(uint8_t *p) { *p = 7; return 0x5A; }

int main(void) {
    uint8_t *r = (uint8_t *)0xC0C0;
    uint8_t a = 0x5A, b = 0x0F;
    buf[0] = 0x5A; xr(buf, 0xFF);
    r[0] = buf[0] == 0xA5;
    buf[1] = 4; ad(buf + 1);
    r[1] = buf[1] == 7;
    copy_then_mark(buf + 4, src, 4);
    r[2] = buf[4] == 0x99 && buf[5] == 2 && buf[6] == 3 && buf[7] == 4;
    buf[8] = 0; buf[9] = 0; merge(buf + 8, 1);
    r[3] = buf[8] == 0x22 && buf[9] == 0x11;
    buf[8] = 0; buf[9] = 0; merge(buf + 8, 0);
    r[4] = buf[8] == 0x22 && buf[9] == 0;
    buf[13] = 4; across_call(buf + 10);
    r[5] = buf[10] == 0x11 && buf[13] == 5;
    r[6] = swap_first(&a, &b) == 0x5A && a == 0x0F && b == 0x5A;
    vp = src;
    r[7] = wide(vp) == 0x01021234UL && wide2(vp + 2, 1) == 0x00030001UL && wide2(vp, 0) == 0x00AB0001UL;
    r[8] = word(vp + 4) == 0x0534;
    r[9] = byte(buf + 14) == 0x5A && buf[14] == 7;
    r[10] = 0xAA;
    return 0;
}
