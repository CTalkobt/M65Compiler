/* test_frame_large_xemu.c — large stack frames (pushed in a loop, dropped by
 * setting SP from the frame pointer)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a recursive function with a 40-byte local array: every level's
 *       array keeps its values
 *   [1] a 300-byte frame (more than 255 words: two push loops)
 *   [2] a variadic function with a 64-byte buffer
 *   [3] the stack pointer is back where it was after all of them
 *   [4] the return value (long, A/X/Y/Z) survives the frame drop
 *   [5] parameters beyond a 300-byte frame (offsets past 255), read after a
 *       call, and variadic arguments beyond a 300-byte buffer
 *   [6] $AA sentinel
 */
#include <stdarg.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned rec(unsigned char n) {
    unsigned char a[40];
    unsigned char i;
    unsigned s;
    for (i = 0; i < 40; i++) a[i] = n + i;
    s = n ? rec(n - 1) : 0;
    for (i = 0; i < 40; i++) if (a[i] != (unsigned char)(n + i)) return 0xFFFF;
    return s + a[39];
}

#pragma cc45 recurse
static unsigned big(unsigned char k) {
    unsigned char b[300];
    unsigned i, s = 0;
    for (i = 0; i < 300; i++) b[i] = (unsigned char)(i + k);
    for (i = 0; i < 300; i++) s += b[i];
    return s;
}

static unsigned var(unsigned char n, ...) {
    unsigned char buf[64];
    unsigned char i;
    unsigned s = 0;
    va_list ap;
    va_start(ap, n);
    for (i = 0; i < n; i++) buf[i] = (unsigned char)va_arg(ap, int);
    va_end(ap);
    for (i = 0; i < n; i++) s += buf[i];
    return s;
}

#pragma cc45 recurse
static unsigned long lng(unsigned char k) {
    unsigned char b[20];
    unsigned char i;
    for (i = 0; i < 20; i++) b[i] = k;
    return 0x12345678UL + b[19];
}


__attribute__((noinline)) static unsigned id(unsigned x) { return x; }

#pragma cc45 recurse
static unsigned bigp(unsigned a, unsigned char b, unsigned long c) {
    unsigned char buf[300];
    unsigned i, s = 0;
    for (i = 0; i < 300; i++) buf[i] = (unsigned char)i;
    for (i = 0; i < 300; i++) s += buf[i];
    s += id(a) + b + (unsigned)c;
    return s + buf[299] + buf[0];
}

static unsigned bigv(unsigned char n, ...) {
    unsigned char buf[300];
    unsigned i, s = 0;
    va_list ap;
    for (i = 0; i < 300; i++) buf[i] = 1;
    va_start(ap, n);
    for (i = 0; i < n; i++) s += va_arg(ap, unsigned);
    va_end(ap);
    return s + buf[0] + buf[299];
}

int main(void) {
    unsigned s0, s1;
    unsigned expect = 0, i;
    unsigned char spl0, sph0;
    __asm__("tsx\n stx $C0E0\n tsy\n sty $C0E1");
    spl0 = *(volatile unsigned char *)0xC0E0; sph0 = *(volatile unsigned char *)0xC0E1;
    /* rec(n) = rec(n-1) + n + 39 */
    for (i = 0; i <= 5; i++) expect += i + 39;
    OUT[0] = rec(5) == expect;
    s0 = 0; for (i = 0; i < 300; i++) s0 += (unsigned char)(i + 7);
    OUT[1] = big(7) == s0;
    OUT[2] = var(4, 10, 20, 30, 40) == 100;
    __asm__("tsx\n stx $C0E0\n tsy\n sty $C0E1");
    OUT[3] = *(volatile unsigned char *)0xC0E0 == spl0 && *(volatile unsigned char *)0xC0E1 == sph0;
    OUT[4] = lng(5) == 0x1234567DUL;
    s1 = 0; for (i = 0; i < 300; i++) s1 += (unsigned char)i;
    OUT[5] = bigp(1000, 7, 0x51234UL) == s1 + 1000 + 7 + 0x1234 + 43 && bigv(3, 100, 200, 300) == 602;
    OUT[6] = 0xAA;
    return 0;
}
