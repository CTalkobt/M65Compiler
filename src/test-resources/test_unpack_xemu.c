/* test_unpack_xemu.c — <unpack.h> (#345): streams pack45 made (test_xemu.sh
 * writes build/test/unpack_stream.h from the pattern below) unpacked into
 * chip RAM, in place, and into attic RAM.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 unpacked to bank 4 from bank 0, the bytes and the count
 *   +1 in place: the stream copied into bank 4 at its in-place offset
 *   +2 to attic RAM ($8000000)
 *   +3 a stream of another format: 0, nothing written
 *   +4 unpack_to: the chip stream to $50000 (its header's $40000 ignored)
 *   +5 unpack_to of another format: 0                        +6 $AA
 */
#include <unpack.h>
#include <dma.h>
#include "unpack_stream.h"   /* stream_chip[] (to $40000), STREAM_CHIP_INPLACE, stream_attic[] */

#define OUT ((volatile unsigned char *)0xC0C0)
#define N 5000

/* the data packed (test_xemu.sh computes the same) */
static unsigned char expect(unsigned int i) {
    unsigned int block = (i >> 8) % 3;
    if (block == 0) return (unsigned char)(i * 7);
    if (block == 1) return (unsigned char)(0x41 + i % 13);
    return 0x55;
}

static unsigned char check(unsigned long at) {
    const unsigned char __far *p = (const unsigned char __far *)at;
    unsigned int i;
    for (i = 0; i < N; i++)
        if (p[i] != expect(i)) return 0;
    return 1;
}

int main(void) {
    static unsigned char bad[8] = {2, 0, 0, 4, 0, 0xC0, 0, 0};
    OUT[0] = unpack((unsigned long)(unsigned int)stream_chip) == N && check(0x40000UL);

    dma_fill_far(0x40000UL, N, 0);
    dma_copy_far(0x40000UL + STREAM_CHIP_INPLACE, (unsigned long)(unsigned int)stream_chip, sizeof stream_chip);
    OUT[1] = unpack(0x40000UL + STREAM_CHIP_INPLACE) == N && check(0x40000UL);

    OUT[2] = unpack((unsigned long)(unsigned int)stream_attic) == N && check(0x8000000UL);

    dma_fill_far(0x40000UL, 16, 0x77);
    OUT[3] = unpack((unsigned long)(unsigned int)bad) == 0 && *(unsigned char __far *)0x40000UL == 0x77;

    dma_fill_far(0x40000UL, N, 0);
    OUT[4] = unpack_to((unsigned long)(unsigned int)stream_chip, 0x50000UL) == N && check(0x50000UL)
             && *(unsigned char __far *)0x40000UL == 0;
    OUT[5] = unpack_to((unsigned long)(unsigned int)bad, 0x50000UL) == 0;

    OUT[6] = 0xAA;
    return 0;
}
