/* test_load_far_xemu.c — <load.h> (#444): files from the D81 test_xemu.sh
 * makes (LDATA: N bytes of the pattern below as a PRG; LPACK: the same
 * packed by pack45 to unpack at $48000) loaded to chip RAM, to attic RAM
 * through the stage, and unpacked; and the errors.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 load_far to chip RAM ($40000): N bytes, load_error 0, the bytes
 *   +1 load_far to attic RAM ($8000100) through the stage
 *   +2 load_unpack: LPACK loaded at $50000, unpacked to $48000
 *   +3 a file not on the disk: 0, load_error 4
 *   +4 a file larger than the stage: 0, LOAD_TOO_BIG
 *   +5 load_unpack of a file that is not packed: 0, LOAD_NOT_PACKED
 *   +6 $AA
 */
#include <load.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define N 3000

/* the data (test_xemu.sh writes the same) */
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
    unsigned char i;
    for (i = 0; i < 7; i++) OUT[i] = 0;

    OUT[0] = load_far("ldata", 5, 0, 0x40000UL) == N && load_error == 0 && check(0x40000UL);

    dma_fill_far(0x8000100UL, N, 0);
    OUT[1] = load_far("ldata", 5, 8, 0x8000100UL) == N && load_error == 0 && check(0x8000100UL);

    dma_fill_far(0x48000UL, N, 0);
    OUT[2] = load_unpack("lpack", 5, 0, 0x50000UL) == N && load_error == 0 && check(0x48000UL);

    OUT[3] = load_far("nosuch", 6, 0, 0x40000UL) == 0 && load_error == 4;

    load_stage(0x40000UL, 1000);
    OUT[4] = load_far("ldata", 5, 0, 0x8000100UL) == 0 && load_error == LOAD_TOO_BIG;
    load_stage(0x40000UL, 0);

    OUT[5] = load_unpack("ldata", 5, 0, 0x40000UL) == 0 && load_error == LOAD_NOT_PACKED;

    OUT[6] = 0xAA;
    return 0;
}
