/* test_dma_mem_xemu.c — memcpy, memmove and memset by DMA (#325), and
 * dma_copy_far / dma_fill_far in asm (#319)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] memmove within one buffer, 300 bytes, one byte up (downwards copy)
 *       and one byte down (upwards), and 1000 bytes 5 up / 5 down
 *   [1] lengths 0 (nothing written) and 1, for all three
 *   [2] memcpy of 300 bytes returns dest (it returned dest + 256)
 *   [3] memset of 300 bytes, and its return value
 *   [4] memset to colour RAM at $D800 (I/O, as the CPU sees it)
 *   [5] dma_copy_far to bank 4 and back, dma_fill_far there; length 0
 *   [6] $AA sentinel
 */
#include <string.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char a[1100], b[400];

static void ramp(unsigned char *p, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++) p[i] = (unsigned char)(i * 7 + 3);
}

/* p[i] == the ramp's value at i + d for i in [from, to) */
static unsigned char ramped(const unsigned char *p, unsigned int from, unsigned int to, int d) {
    unsigned int i;
    for (i = from; i < to; i++)
        if (p[i] != (unsigned char)((i + d) * 7 + 3)) return 0;
    return 1;
}

int main(void) {
    unsigned char ok;
    unsigned int i;

    ramp(a, 1100);
    memmove(a + 1, a, 300);                     /* up: a[1..300] = old a[0..299] */
    ok = a[0] == 3 && ramped(a, 1, 301, -1) && ramped(a, 301, 1100, 0);
    ramp(a, 1100);
    memmove(a, a + 1, 300);                     /* down */
    ok &= ramped(a, 0, 300, 1) && ramped(a, 300, 1100, 0);
    ramp(a, 1100);
    memmove(a + 5, a, 1000);
    ok &= ramped(a, 0, 5, 0) && ramped(a, 5, 1005, -5);
    ramp(a, 1100);
    memmove(a, a + 5, 1000);
    ok &= ramped(a, 0, 1000, 5) && ramped(a, 1000, 1100, 0);
    OUT[0] = ok;

    ramp(a, 16);
    ok = memmove(a + 1, a, 0) == a + 1 && memcpy(a + 2, a + 9, 0) == a + 2 && memset(a, 0xEE, 0) == a;
    ok &= ramped(a, 0, 16, 0);
    memmove(a + 1, a, 1);
    memcpy(a + 5, a + 9, 1);
    memset(a + 7, 0xEE, 1);
    ok &= a[1] == 3 && a[0] == 3 && a[2] == 17 && a[5] == (unsigned char)(9 * 7 + 3) &&
          a[6] == (unsigned char)(6 * 7 + 3) && a[7] == 0xEE && a[8] == (unsigned char)(8 * 7 + 3);
    OUT[1] = ok;

    ramp(a, 300);
    OUT[2] = memcpy(b, a, 300) == b && ramped(b, 0, 300, 0);

    ok = memset(b + 50, 0x5A, 300) == b + 50;
    for (i = 50; i < 350; i++) if (b[i] != 0x5A) ok = 0;
    OUT[3] = ok && ramped(b, 0, 50, 0) && b[350] == 0;

    memset((void *)0xD800, 5, 40);
    ok = 1;
    for (i = 0; i < 40; i++) if ((((volatile unsigned char *)0xD800)[i] & 0x0F) != 5) ok = 0;
    OUT[4] = ok;

    ramp(a, 1000);
    dma_copy_far(0x41000UL, (unsigned long)(unsigned int)a, 1000);
    memset(a, 0, 1000);
    dma_copy_far((unsigned long)(unsigned int)a, 0x41000UL, 1000);
    ok = ramped(a, 0, 1000, 0);
    dma_fill_far(0x41000UL, 300, 0x77);
    dma_fill_far(0x41000UL, 0, 0x11);                   /* nothing */
    dma_copy_far((unsigned long)(unsigned int)b, 0x41000UL, 0);
    dma_copy_far((unsigned long)(unsigned int)b, 0x41000UL, 301);
    for (i = 0; i < 300; i++) if (b[i] != 0x77) ok = 0;
    OUT[5] = ok && b[300] == (unsigned char)(300 * 7 + 3);
    OUT[6] = 0xAA;
    return 0;
}
