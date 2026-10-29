/* benchmark_tgi.c — <tgi.h>'s speed: frames (clock()) for each workload,
 * and a checksum of the page after it (the pixels drawn must not change
 * when the code does)
 *
 * src/test/bench_tgi.sh builds and runs it in xemu and prints the table.
 * Results at $C0C0: for each workload its frames (2 bytes), its checksum
 * (2 bytes); then $AA.
 */
#include <tgi.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define NBENCH 8

static clock_t t0;

/* The draw page's checksum: a rotate-and-add over its 64000 bytes */
static unsigned int checksum(void) {
    unsigned long a = 0;
    const unsigned char __far *p;
    unsigned int s = 0, i;
    tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &a);
    p = (const unsigned char __far *)a;
    for (i = 0; i < 64000u; i++) s = (unsigned int)((s << 1) | (s >> 15)) + p[i];
    return s;
}

static void start(void) {
    tgi_clear();
    t0 = clock();
}

static void stop(unsigned char k) {
    unsigned int f = (unsigned int)(clock() - t0), s = checksum();
    OUT[4 * k] = (unsigned char)f;
    OUT[4 * k + 1] = (unsigned char)(f >> 8);
    OUT[4 * k + 2] = (unsigned char)s;
    OUT[4 * k + 3] = (unsigned char)(s >> 8);
}

/* the driver (bench_tgi.sh -DTGI_DRIVER=tgi_static_ncm640: the 640x200 one,
 * drawing the same in its left half) */
#ifndef TGI_DRIVER
#define TGI_DRIVER tgi_static_stddrv
#endif

/* A pseudo-random number (the same each run) */
static unsigned int seed = 12345;
static unsigned int rnd(void) {
    seed = seed * 25173u + 13849u;
    return seed;
}

int main(void) {
    signed int x, y, i, n, r;
    unsigned int sum = 0;
    unsigned char k;
    for (k = 0; k < 4 * NBENCH; k++) OUT[k] = 0;
    OUT[4 * NBENCH] = 0xAA;
    tgi_install(TGI_DRIVER);
    tgi_init();

    /* 0: tgi_setpixel, every pixel of the screen, twice */
    start();
    for (r = 0; r < 2; r++)
    for (y = 0; y < 200; y++)
        for (x = 0; x < 320; x++) {
            tgi_setcolor((unsigned char)(x ^ y));
            tgi_setpixel(x, y);
        }
    stop(0);

    /* 1: tgi_getpixel, every pixel (of a screen with a pattern), twice */
    start();
    tgi_setcolor(7);
    tgi_bar(40, 40, 279, 159);
    t0 = clock();
    for (r = 0; r < 2; r++)
    for (y = 0; y < 200; y++)
        for (x = 0; x < 320; x++) sum += tgi_getpixel(x, y);
    stop(1);
    tgi_setcolor((unsigned char)sum);   /* (keeps the loop) */

    /* 2: long lines: from the centre to every 2nd pixel of the edge, 4
     * times */
    start();
    for (r = 0; r < 4; r++) {
    tgi_setcolor((unsigned char)(r + 1));
    for (x = 0; x < 320; x += 2) {
        tgi_line(160, 100, x, 0);
        tgi_line(160, 100, x, 199);
    }
    for (y = 0; y < 200; y += 2) {
        tgi_line(160, 100, 0, y);
        tgi_line(160, 100, 319, y);
    }
    }
    stop(2);

    /* 3: short lines: 12000 of up to 15 pixels, any direction */
    start();
    for (r = 0; r < 4; r++)
    for (i = 0; i < 3000; i++) {
        x = (signed int)(rnd() % 300u) + 10;
        y = (signed int)(rnd() % 180u) + 10;
        tgi_setcolor((unsigned char)i);
        tgi_line(x, y, x + (signed int)(rnd() & 15) - 7, y + (signed int)(rnd() & 15) - 7);
    }
    stop(3);

    /* 4: circles: 60, radii 5..95, 4 times */
    start();
    for (r = 0; r < 4; r++)
    for (i = 0; i < 60; i++) {
        tgi_setcolor((unsigned char)(i + 1));
        tgi_circle(160, 100, (unsigned char)(5 + (i * 3) / 2));
    }
    stop(4);

    /* 5: bitmap text at 1x: the screen full (40 x 25), 4 times */
    start();
    for (n = 0; n < 4; n++) {
        tgi_setcolor((unsigned char)(n + 1));
        for (y = 0; y < 200; y += 8)
            tgi_outtextxy(0, y, "The quick brown fox jumps over the lazy!");
    }
    stop(5);

    /* 6: bitmap text at 2x: the screen full (20 x 12), 8 times */
    start();
    tgi_settextscale(0x200, 0x200);
    for (n = 0; n < 8; n++) {
        tgi_setcolor((unsigned char)(n + 3));
        for (y = 0; y < 192; y += 16)
            tgi_outtextxy(0, y, "Pack my box with fiv");
    }
    tgi_settextscale(0x100, 0x100);
    stop(6);

    /* 7: bars: 8000 of up to 32 x 16 */
    start();
    for (r = 0; r < 8; r++)
    for (i = 0; i < 1000; i++) {
        x = (signed int)(rnd() % 288u);
        y = (signed int)(rnd() % 184u);
        tgi_setcolor((unsigned char)i);
        tgi_bar(x, y, x + (signed int)(rnd() & 31), y + (signed int)(rnd() & 15));
    }
    stop(7);

    tgi_uninstall();
    return 0;
}
