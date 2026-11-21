/* test_unpack2only_xemu.c — <unpack.h> with its formats chosen (#666): this
 * program links format 2's decoder alone (UNPACK_EXPLICIT_FORMATS,
 * UNPACK_FORMAT2), so a format 1 stream is not unpacked. test_xemu.sh also
 * checks its map has no format 1 decoder. Streams from gen_unpack2_stream.sh.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 a (format 2) to $40000: the bytes and the count
 *   +1 a's bytes as a format 1 stream: UNPACK_ERROR, nothing written
 *   +2 $AA
 */
#define UNPACK_EXPLICIT_FORMATS
#include <unpack.h>
#include <dma.h>
#include "unpack2_stream.h"

UNPACK_FORMAT2;

#define OUT ((volatile unsigned char *)0xC0C0)
#define ADDR(a) ((unsigned long)(unsigned int)(a))

static unsigned char same(unsigned long at, const unsigned char *expect, unsigned int n) {
    const unsigned char __far *p = (const unsigned char __far *)at;
    unsigned int i;
    for (i = 0; i < n; i++)
        if (p[i] != expect[i]) return 0;
    return 1;
}

int main(void) {
    dma_fill_far(0x40000UL, 2000, 0xEE);
    OUT[0] = unpack(ADDR(a_chip)) == sizeof a_img && same(0x40000UL, a_img, sizeof a_img);

    dma_fill_far(0x40000UL, 2000, 0xEE);
    OUT[1] = unpack(ADDR(a1_chip)) == UNPACK_ERROR && *(unsigned char __far *)0x40000UL == 0xEE;

    OUT[2] = 0xAA;
    return 0;
}
