/* clock.c — clock(): video frames since power-on
 *
 * The C65 KERNAL has no C64-style jiffy clock (its RDTIM reads the CIA
 * time-of-day clock in BCD). Its raster interrupt decrements a 24-bit
 * counter at $110C-$110E once per frame, starting from 0 at reset, so the
 * frames elapsed are 2^24 minus its value. CLOCKS_PER_SEC is the frame
 * rate: 50 (PAL) or 60 (NTSC).
 */
#include <time.h>

clock_t clock(void) {
    volatile unsigned char *c = (volatile unsigned char *)0x110C;
    unsigned long a, b;
    /* The interrupt may change it between the byte reads: read until two
       reads agree */
    do {
        a = c[0] | ((unsigned int)c[1] << 8) | ((unsigned long)c[2] << 16);
        b = c[0] | ((unsigned int)c[1] << 8) | ((unsigned long)c[2] << 16);
    } while (a != b);
    return (0x1000000UL - a) & 0xFFFFFFUL;
}
