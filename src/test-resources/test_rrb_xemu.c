/* test_rrb_xemu.c — <rrb.h> (#645): 16-bit character rows and GOTOX
 * tokens, checked in the VIC-IV's registers and the cells' bytes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] rrb_setup(60): CHR16 on, CHRCOUNT 60, LINESTEP 120; rrb_done(40):
 *       off, 40 and 40; a CHRCOUNT over 255 (its bits 8-9 in $D063 4-5)
 *   [1] rrb_screen_cell / rrb_colour_cell: the screen and colour RAM
 *       pointers + row * LINESTEP + col * 2
 *   [2] rrb_gotox: x's 10 bits in the screen bytes; colour byte 0 bit 4
 *       (and RRB_TRANSPARENT, RRB_ROWMASK with the mask in byte 1)
 *   [3] rrb_putchar: a 13-bit character, its colour in colour byte 1
 *   [4] rrb_double: $D051 bit 6 on and off
 *   [5] $AA sentinel
 */
#include <rrb.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char rd[2];

static void peek2(unsigned long a) {
    dma_copy_far((unsigned long)(unsigned int)rd, a, 2);
}

int main(void) {
    unsigned char d054 = REG(0xD054), d05e = REG(0xD05E), d058 = REG(0xD058), d059 = REG(0xD059);
    unsigned long scr = REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16);
    unsigned long col = 0xFF80000UL + (REG(0xD064) | ((unsigned int)REG(0xD065) << 8));
    int ok;
    OUT[5] = 0;

    rrb_setup(60);
    ok = (REG(0xD054) & 1) && REG(0xD05E) == 60 && REG(0xD058) == 120 && REG(0xD059) == 0;
    rrb_setup(300);
    ok = ok && REG(0xD05E) == (300 & 0xFF) && (REG(0xD063) & 0x30) == 0x10 && REG(0xD058) == (600 & 0xFF) &&
         REG(0xD059) == 2;
    rrb_setup(60);
    ok = ok && (REG(0xD063) & 0x30) == 0;
    OUT[0] = ok;

    ok = rrb_screen_cell(0, 0) == scr && rrb_screen_cell(2, 5) == scr + 240 + 10 &&
         rrb_colour_cell(2, 5) == col + 250;
    OUT[1] = ok;

    rrb_gotox(1, 3, 0x2A5, RRB_TRANSPARENT, 0);
    peek2(rrb_screen_cell(1, 3));
    ok = rd[0] == 0xA5 && rd[1] == 0x02;
    peek2(rrb_colour_cell(1, 3));
    ok = ok && rd[0] == 0x90 && rd[1] == 0x00;
    rrb_gotox(1, 4, 16, RRB_ROWMASK, 0x0F);
    peek2(rrb_colour_cell(1, 4));
    ok = ok && rd[0] == 0x18 && rd[1] == 0x0F;
    OUT[2] = ok;

    rrb_putchar(1, 5, 0x1234, 7);
    peek2(rrb_screen_cell(1, 5));
    ok = rd[0] == 0x34 && rd[1] == 0x12;
    peek2(rrb_colour_cell(1, 5));
    ok = ok && rd[0] == 0 && rd[1] == 7;
    OUT[3] = ok;

    rrb_double(1);
    ok = (REG(0xD051) & 0x40) != 0;
    rrb_double(0);
    ok = ok && !(REG(0xD051) & 0x40);
    OUT[4] = ok;

    rrb_done(40);
    ok = !(REG(0xD054) & 1) && REG(0xD05E) == 40 && REG(0xD058) == 40;
    /* the screen as it was */
    REG(0xD054) = d054;
    REG(0xD05E) = d05e;
    REG(0xD058) = d058;
    REG(0xD059) = d059;
    OUT[0] = OUT[0] && ok;
    OUT[5] = 0xAA;
    return 0;
}
