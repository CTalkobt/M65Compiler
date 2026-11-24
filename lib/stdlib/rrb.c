/* rrb.c — <rrb.h>: 16-bit character rows with GOTOX tokens (#645) */

#include <rrb.h>
#include <dma.h>

#define REG(a) (*(volatile unsigned char *)(a))

static void rrb_rows(unsigned int chrcount, unsigned int linestep) {
    REG(0xD05E) = (unsigned char)chrcount;
    REG(0xD063) = (unsigned char)((REG(0xD063) & 0xCF) | ((chrcount >> 4) & 0x30));
    REG(0xD058) = (unsigned char)linestep;
    REG(0xD059) = (unsigned char)(linestep >> 8);
}

void rrb_setup(unsigned int chrcount) {
    REG(0xD054) |= 0x01;            /* CHR16 */
    rrb_rows(chrcount, chrcount * 2);
}

void rrb_done(unsigned int chrcount) {
    REG(0xD054) &= 0xFE;
    rrb_rows(chrcount, chrcount);
}

void rrb_double(unsigned char on) {
    if (on) REG(0xD051) |= 0x40;
    else REG(0xD051) &= 0xBF;
}

static unsigned int rrb_off(unsigned int row, unsigned int col) {
    return row * (REG(0xD058) | ((unsigned int)REG(0xD059) << 8)) + col * 2;
}

unsigned long rrb_screen_cell(unsigned int row, unsigned int col) {
    unsigned long scr = REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16) |
                        ((unsigned long)(REG(0xD063) & 0x0F) << 24);
    return scr + rrb_off(row, col);
}

unsigned long rrb_colour_cell(unsigned int row, unsigned int col) {
    return 0xFF80000UL + (REG(0xD064) | ((unsigned int)REG(0xD065) << 8)) + rrb_off(row, col);
}

static void put2(unsigned long addr, unsigned char a, unsigned char b) {
    unsigned char v[2];
    v[0] = a;
    v[1] = b;
    dma_copy_far(addr, (unsigned long)(unsigned int)v, 2);
}

void rrb_gotox(unsigned int row, unsigned int col, unsigned int x, unsigned char flags, unsigned char mask) {
    put2(rrb_screen_cell(row, col), (unsigned char)x, (unsigned char)((x >> 8) & 0x03));
    put2(rrb_colour_cell(row, col), (unsigned char)(0x10 | (flags & (RRB_TRANSPARENT | RRB_ROWMASK))),
         (flags & RRB_ROWMASK) ? mask : 0);
}

void rrb_putchar(unsigned int row, unsigned int col, unsigned int ch, unsigned char colour) {
    put2(rrb_screen_cell(row, col), (unsigned char)ch, (unsigned char)((ch >> 8) & 0x1F));
    put2(rrb_colour_cell(row, col), 0, colour);
}
