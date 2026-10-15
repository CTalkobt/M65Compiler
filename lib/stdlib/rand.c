/* rand.c — rand / srand
 *
 * Until srand is called, rand reads the MEGA65's hardware random number
 * generator (every run differs). srand(seed) switches to C's reference
 * generator (next = next * 1103515245 + 12345), so a seed repeats its
 * sequence. Results are 0 to RAND_MAX (32767).
 */
#include <stdlib.h>

static unsigned long __rand_next;
static unsigned char __rand_seeded;

__fastcall__ int rand(void) {
    if (!__rand_seeded) {
        /* $D7EF: the next random byte, ready when bit 7 of $D7FE is clear */
        volatile unsigned char *busy = (volatile unsigned char *)0xD7FE;
        volatile unsigned char *rng = (volatile unsigned char *)0xD7EF;
        unsigned char lo, hi;
        while (*busy & 0x80) ;
        lo = *rng;
        while (*busy & 0x80) ;
        hi = *rng & 0x7F;
        return (int)(((unsigned int)hi << 8) | lo);
    }
    __rand_next = __rand_next * 1103515245UL + 12345UL;
    return (int)((__rand_next >> 16) & 0x7FFF);
}

__fastcall__ void srand(unsigned int seed) {
    __rand_next = seed;
    __rand_seeded = 1;
}
