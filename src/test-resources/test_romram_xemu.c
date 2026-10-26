/* test_romram_xemu.c — <rom_ram.h> (#339): the C64 ROM space as RAM
 *
 * Results at $C0C0 (01 = pass):
 *   [0] romram_ranges: ROMRAM_C64's two ranges; NULL for an unknown
 *       region; romram_enable of no region or an unknown one is 0
 *   [1] protected (C65 mode's default): a DMA write to $2A000 is ignored
 *   [2] romram_enable(ROMRAM_C64) is 1; the protection bit ($D67D bit 2)
 *       is clear; DMA writes to the ends of both ranges read back
 *   [3] romram_protect: the bit set again, writes ignored, the contents kept
 *   [4] __romram_exit (crt0's __exit calls it) protects what was left
 *       writable; writes left on at the end: the program still returns
 *   [5] $AA sentinel
 */
#include <rom_ram.h>
#include <dma.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define HCPU (*(volatile unsigned char *)0xD67D)

static unsigned char buf[16], back[16];

asm(".extern __romram_exit");

static void put(unsigned long at, unsigned char v) {
    memset(buf, v, 16);
    dma_copy_far(at, (unsigned long)(unsigned)buf, 16);
}

static unsigned char is(unsigned long at, unsigned char v) {
    unsigned char i;
    memset(back, (unsigned char)~v, 16);
    dma_copy_far((unsigned long)(unsigned)back, at, 16);
    for (i = 0; i < 16; i++) if (back[i] != v) return 0;
    return 1;
}

int main(void) {
    const unsigned long *r = romram_ranges(ROMRAM_C64);
    unsigned char ok;

    OUT[0] = r && r[0] == 2 && r[1] == 0x2A000UL && r[2] == 0x2BFFFUL && r[3] == 0x2E000UL && r[4] == 0x2FFFFUL &&
             romram_ranges(2) == 0 && romram_enable(0) == 0 && romram_enable(2) == 0;

    {
        /* the ROM's bytes there, a write of their complement, the same bytes */
        static unsigned char rom[16];
        unsigned char i;
        dma_copy_far((unsigned long)(unsigned)rom, 0x2A000UL, 16);
        for (i = 0; i < 16; i++) buf[i] = (unsigned char)~rom[i];
        dma_copy_far(0x2A000UL, (unsigned long)(unsigned)buf, 16);
        dma_copy_far((unsigned long)(unsigned)back, 0x2A000UL, 16);
        OUT[1] = memcmp(back, rom, 16) == 0 && (HCPU & 4) != 0;
    }

    ok = romram_enable(ROMRAM_C64) == 1 && !(HCPU & 4);
    put(0x2A000UL, 0x5A);
    put(0x2BFF0UL, 0x5B);
    put(0x2E000UL, 0x5C);
    put(0x2FFF0UL, 0x5D);
    if (!is(0x2A000UL, 0x5A) || !is(0x2BFF0UL, 0x5B) || !is(0x2E000UL, 0x5C) || !is(0x2FFF0UL, 0x5D)) ok = 0;
    OUT[2] = ok;

    romram_protect();
    ok = (HCPU & 4) != 0;
    put(0x2A000UL, 0xA5);
    put(0x2FFF0UL, 0xA5);
    if (!is(0x2A000UL, 0x5A) || !is(0x2FFF0UL, 0x5D)) ok = 0;
    OUT[3] = ok;

    /* crt0's exit hook: protects what was left writable */
    ok = romram_enable(ROMRAM_C64) == 1 && !(HCPU & 4);
    asm("jsr __romram_exit");
    if (!(HCPU & 4)) ok = 0;
    OUT[4] = ok && romram_enable(ROMRAM_C64);   /* (left on: __exit protects it) */
    OUT[5] = 0xAA;
    return 0;
}
