/* test_stack16_xemu.c — the 16-bit CPU stack (E clear, SP topped at $BFFF)
 *
 * Results at $C0C0:
 *   [0] SPH inside main ($BF by default, $01 with -fno-stack16)
 *   [1] E flag inside main ($00 by default, $20 with -fno-stack16)
 *   [2] recursion DEPTH deep returns DEPTH (> 256 bytes of stack by default),
 *       with ROM IRQs taken at the bottom
 *   [3] variable-size alloca in a recursive function (asm alloca)
 *   [4] setjmp/longjmp through a nested call
 *   [5] sprintf %d
 *   [6] float multiply (BASIC ROM via JSRFAR)
 *   [7] KERNAL CHROUT (putchar) returns
 *   [8] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifndef DEPTH
#define DEPTH 60     /* -fno-stack16: 4 (page 1 holds 256 bytes) */
#endif
#ifndef VA_N
#define VA_N 130     /* -fno-stack16: 50 */
#endif

#define OUT ((volatile unsigned char *)0xC0C0)
#define RASTER (*(volatile unsigned char *)0xD012)

void *alloca(unsigned int size);

unsigned char sph, pflags;
static jmp_buf jb;

__attribute__((noinline)) void probe(void) {
    __asm__("tsy");
    __asm__("sty _sph");
    __asm__("php");
    __asm__("pla");
    __asm__("sta _pflags");
}

static void wait_frames(int n) {
    while (n--) {
        while (RASTER != 0) {}
        while (RASTER == 0) {}
    }
}

int deep(int n) {
    volatile char pad[16];
    pad[0] = n;
    if (n == 0) { wait_frames(3); return 0; }
    return deep(n - 1) + pad[0] - n + 1;
}

#pragma cc45 recurse
int va(int n) {
    char *p = alloca(n);
    memset(p, 7, n);
    if (n > 40) return va(n - 40) + p[0] + p[n - 1];
    return p[0] + p[n - 1];
}

void jumper(int v) { longjmp(jb, v); }

int main(void) {
    char buf[16];
    float f = 1.5f;
    int r;
    probe();
    OUT[0] = sph;
    OUT[1] = pflags & 0x20;
    r = deep(DEPTH);
    OUT[2] = r == DEPTH;
    r = va(VA_N);
    OUT[3] = r == 14 * ((VA_N + 39) / 40);
    r = setjmp(jb);
    if (r == 0) jumper(7);
    OUT[4] = r == 7;
    sprintf(buf, "%d", 12345);
    OUT[5] = strcmp(buf, "12345") == 0;
    f = f * 3.0f;
    OUT[6] = (int)f == 4;
    putchar('A');
    OUT[7] = 1;
    OUT[8] = 0xAA;
    return 0;
}
