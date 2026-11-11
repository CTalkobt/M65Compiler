/* test_font_xemu.c — <font.h> (#466) with fonts font45 made from
 * src/test-resources/font45/tiny.txt (test_xemu.sh: tiny.c, a proportional
 * 1 bpp font in PETSCII, cc45's literals: codes 32-201, A at 193; tiny2.c, A
 * alone at 2 bpp, fixed).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] font_from: the header read; data that is not a font gives NULL
 *   [1] font_glyph_width / font_width: A 4, I 1, a space 2, Z (not in the
 *       font) 0; "AB I" 11
 *   [2] font_draw_bitmap: "AI" at 2,1 in a 32-byte-pitch buffer in bank 4,
 *       A's rows and I's column in palette entry 1, nothing else; the x after
 *   [3] font_ramp: a 2 bpp glyph's level 3 in entry first + 2
 *   [4] font_clip: a glyph at x -1 (signed) loses its first column, nothing
 *       written before the buffer; a second A past x1 not drawn
 *   [5] font_install copies by DMA; font_select sets $D068-$D06A
 *   [6] $AA sentinel
 */
#include <font.h>
#include <dma.h>
#include "tiny.h"
#include "tiny2.h"
#define OUT ((volatile unsigned char *)0xC0C0)
#define LIN(p) ((unsigned long)(unsigned int)(p))
#define BUF 0x40100UL
#define PITCH 32

static unsigned char px(signed int x, signed int y) {
    return *(const unsigned char __far *)(BUF + (unsigned long)(y * PITCH) + x);
}
static unsigned char at(unsigned long a) { return *(const unsigned char __far *)a; }
static void clear(void) { dma_fill_far(BUF - 0x100, 0x500, 0); }
static const unsigned char notfont[12] = { 'F', '5', 1 };
static const unsigned char cs[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

int main(void) {
    font_t f, f2, bad;
    unsigned char ok, i, y, s0, s1, s2;
    signed int nx;
    /* [0] */
    OUT[0] = font_from(&f, LIN(tiny)) == &f && f.height == 5 && f.baseline == 5 && f.first == 32 &&
             f.last == 201 && f.bpp == 1 && (f.flags & FONT_PROPORTIONAL) && f.cellw == 4 && f.rowbytes == 1 &&
             font_from(&bad, LIN(notfont)) == 0 &&
             font_from(&f2, LIN(tiny2)) == &f2 && f2.bpp == 2 && !(f2.flags & FONT_PROPORTIONAL) && f2.widths == 0;
    /* [1] */
    OUT[1] = font_glyph_width(&f, 'A') == 4 && font_glyph_width(&f, 'I') == 1 && font_glyph_width(&f, ' ') == 2 &&
             font_glyph_width(&f, 'Z') == 0 && font_width(&f, "AB I") == 11 && font_glyph_width(&f2, 'A') == 4;
    /* [2] A: 0110 1001 1111 1001 1001, I: a column of 5 */
    clear();
    nx = font_draw_bitmap(&f, BUF, PITCH, 2, 1, "AI");
    ok = nx == 7 && px(2, 1) == 0 && px(3, 1) == 1 && px(4, 1) == 1 && px(5, 1) == 0 &&
         px(2, 2) == 1 && px(3, 2) == 0 && px(5, 2) == 1 &&
         px(2, 3) == 1 && px(3, 3) == 1 && px(4, 3) == 1 && px(5, 3) == 1 && px(7, 3) == 0;
    for (y = 1; y <= 5; y++) if (px(6, y) != 1) ok = 0;
    if (px(6, 0) || px(6, 6) || px(2, 0) || px(2, 6) || px(1, 3)) ok = 0;
    OUT[2] = ok;
    /* [3] */
    clear();
    font_ramp(16, 3);
    font_draw_bitmap(&f2, BUF, PITCH, 0, 10, "A");
    OUT[3] = px(0, 10) == 0 && px(1, 10) == 18 && px(2, 10) == 18 && px(0, 12) == 18 && px(3, 12) == 18;
    font_ramp(1, 1);
    /* [4] B: 1110 ...; clip 0,0-3,31 */
    clear();
    font_clip(0, 0, 3, 31);
    nx = font_draw_bitmap(&f, BUF, PITCH, -1, 0, "B");
    ok = nx == 3 && px(0, 0) == 1 && px(1, 0) == 1 && px(2, 0) == 0 && at(BUF - 1) == 0;
    for (i = 1; i < 32; i++) if (at(BUF - i) != 0) ok = 0;
    font_draw_bitmap(&f, BUF, PITCH, 0, 8, "AA");
    ok = ok && px(3, 10) == 1 && px(4, 10) == 0 && px(5, 10) == 0 && px(7, 10) == 0;
    OUT[4] = ok;
    font_clip(0, 0, 32767, 32767);
    /* [5] */
    font_install(LIN(cs), 16, 0x41000UL);
    ok = 1;
    for (i = 0; i < 16; i++) if (at(0x41000UL + i) != i + 1) ok = 0;
    s0 = *(volatile unsigned char *)0xD068; s1 = *(volatile unsigned char *)0xD069; s2 = *(volatile unsigned char *)0xD06A;
    font_select(0x41000UL);
    ok = ok && *(volatile unsigned char *)0xD068 == 0x00 && *(volatile unsigned char *)0xD069 == 0x10 &&
         *(volatile unsigned char *)0xD06A == 0x04;
    *(volatile unsigned char *)0xD068 = s0; *(volatile unsigned char *)0xD069 = s1; *(volatile unsigned char *)0xD06A = s2;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
