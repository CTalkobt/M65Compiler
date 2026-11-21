/* test_unpack2_xemu.c — <unpack.h>'s format 2, images (#666): streams
 * pack45 --image made (test_xemu.sh writes build/test/unpack2_stream.h with
 * src/test/gen_unpack2_stream.sh) unpacked into chip RAM, in place and into
 * attic RAM, compared with the images' bytes.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 a (64x40, 16 colours, from the left) to $40000: the bytes and the count
 *   +1 a in place: the stream copied into bank 4 at its in-place offset
 *   +2 a to attic RAM ($8000000)
 *   +3 b (256x3, 2 colours, from above) to $40000
 *   +4 c (10x5, 1 colour) with unpack_to to $50000 (its header's $40000 ignored)
 *   +5 a stream of format 3: 0, nothing written
 *   +6 $AA
 */
#include <unpack.h>
#include <dma.h>
#include "unpack2_stream.h"

UNPACK_IMAGES;

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
    static unsigned char bad[16] = {3, 0, 0, 4, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0};

    dma_fill_far(0x40000UL, 2000, 0xEE);
    OUT[0] = unpack(ADDR(a_chip)) == sizeof a_img && same(0x40000UL, a_img, sizeof a_img);

    dma_fill_far(0x40000UL, 2000, 0xEE);
    dma_copy_far(0x40000UL + A_INPLACE, ADDR(a_chip), sizeof a_chip);
    OUT[1] = unpack(0x40000UL + A_INPLACE) == sizeof a_img && same(0x40000UL, a_img, sizeof a_img);

    OUT[2] = unpack(ADDR(a_attic)) == sizeof a_img && same(0x8000000UL, a_img, sizeof a_img);

    dma_fill_far(0x40000UL, 2000, 0xEE);
    OUT[3] = unpack(ADDR(b_chip)) == sizeof b_img && same(0x40000UL, b_img, sizeof b_img) &&
             *(unsigned char __far *)(0x40000UL + sizeof b_img) == 0xEE;

    dma_fill_far(0x40000UL, 64, 0xEE);
    dma_fill_far(0x50000UL, 64, 0xEE);
    OUT[4] = unpack_to(ADDR(c_chip), 0x50000UL) == sizeof c_img && same(0x50000UL, c_img, sizeof c_img) &&
             *(unsigned char __far *)(0x50000UL + sizeof c_img) == 0xEE &&
             *(unsigned char __far *)0x40000UL == 0xEE;

    dma_fill_far(0x40000UL, 16, 0x77);
    OUT[5] = unpack(ADDR(bad)) == 0 && *(unsigned char __far *)0x40000UL == 0x77;

    OUT[6] = 0xAA;
    return 0;
}
