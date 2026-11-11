/* test_font_cells_xemu.c — <font.h>'s glyph sets and font_load (#466),
 * with fonts font45 makes (test_xemu.sh, build/test/fontc):
 *   seg.c    font45 --segments 7 --size 16 --bpp 2 --fcm --palette 16,3,
 *            the digits: FCM characters, 2 x 2 a glyph (lit 18, unlit 16)
 *   tiny.c   tiny.txt, 1 bpp proportional (A at 193, PETSCII)
 * and a disk with tiny.txt's font as a file (FONT) and packed (FPACK,
 * to unpack at $46000).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] font_from of a glyph set: FCM, 2 x 2 characters, 64-byte aligned
 *   [1] font_put_cells: each glyph's characters down its columns on a
 *       16-bit screen, the column after
 *   [2] the characters' bytes: '8' all lit (18), '1' lit and unlit (16)
 *   [3] font_draw_fcm: a glyph across two characters, numbered down the
 *       columns (strip 4) and along the rows (strip -10)
 *   [4] font_draw_ncm: nibbles, the left pixel the low one
 *   [5] font_load: a font file, a packed one, a missing one (NULL, error 4)
 *   [6] $AA sentinel
 */
#include <font.h>
#include <dma.h>
#include <load.h>
#include "seg.h"
#include "tiny.h"

#define OUT ((volatile unsigned char *)0xC0C0)
#define LIN(p) ((unsigned long)(unsigned int)(p))
#define SCREEN 0x40000UL
#define CH 0x41000UL
#define CH2 0x42000UL
#define CN 0x43000UL

static unsigned char at(unsigned long a) { return *(const unsigned char __far *)a; }
static unsigned int at16(unsigned long a) { return at(a) | ((unsigned int)at(a + 1) << 8); }

int main(void) {
    font_t s, t;
    const font_t *l;
    unsigned char ok, i, lit, dim, other, col;
    unsigned int n0;
    /* [0] */
    OUT[0] = font_from(&s, LIN(seg)) == &s && (s.flags & FONT_FCM) && s.bpp == 8 && s.cols == 2 && s.rows == 2 &&
             (s.glyphs & 63) == 0 && s.glyphs > LIN(seg) && font_from(&t, LIN(seg) + 1) == 0 &&
             font_from(&t, LIN(tiny)) == &t;
    /* [1] '1' then '0' from column 3, row 2 (80 bytes a row) */
    dma_fill_far(SCREEN, 2000, 0);
    n0 = (unsigned int)(s.glyphs >> 6);
    col = font_put_cells(&s, SCREEN, 80, 3, 2, "10");
    OUT[1] = col == 7 && at16(SCREEN + 160 + 6) == n0 + 4 && at16(SCREEN + 240 + 6) == n0 + 5 &&
             at16(SCREEN + 160 + 8) == n0 + 6 && at16(SCREEN + 240 + 8) == n0 + 7 && at16(SCREEN + 160 + 10) == n0 &&
             at16(SCREEN + 160 + 4) == 0 && at16(SCREEN + 160 + 14) == 0;
    /* [2] '8' (index 8): its 4 characters; '1' (index 1) */
    lit = dim = other = 0;
    for (i = 0; i < 255; i++) {
        unsigned char v = at(s.glyphs + 8 * 256UL + i);
        if (v == 18) lit = 1; else if (v == 16) dim = 1; else if (v) other = 1;
    }
    ok = lit && !dim && !other;
    lit = dim = other = 0;
    for (i = 0; i < 255; i++) {
        unsigned char v = at(s.glyphs + 1 * 256UL + i);
        if (v == 18) lit = 1; else if (v == 16) dim = 1; else if (v) other = 1;
    }
    OUT[2] = ok && lit && dim && !other;
    /* [3] A (0110 1001 1111 1001 1001) at x 6: row 0's pixels 7 and 8 in two characters */
    dma_fill_far(CH, 2048, 0);
    dma_fill_far(CH2, 2048, 0);
    font_draw_fcm(&t, CH, 4, 6, 1, "A");
    font_draw_fcm(&t, CH2, -10, 6, 1, "A");
    OUT[3] = at(CH + 8 + 7) == 1 && at(CH + 8 + 6) == 0 && at(CH + 4 * 64 + 8 + 0) == 1 && at(CH + 4 * 64 + 8 + 1) == 0 &&
             at(CH + 3 * 8 + 6) == 1 && at(CH + 4 * 64 + 3 * 8 + 1) == 1 &&
             at(CH2 + 8 + 7) == 1 && at(CH2 + 64 + 8 + 0) == 1 && at(CH2 + 4 * 64 + 8) == 0;
    /* [4] A at x 1, entry 5: row 0 (0110: pixels 2, 3), row 2 (1111: pixels 1-4) */
    dma_fill_far(CN, 512, 0);
    font_ramp(5, 1);
    font_draw_ncm(&t, CN, 2, 1, 0, "A");
    font_ramp(1, 1);
    OUT[4] = at(CN + 0) == 0x00 && at(CN + 1) == 0x55 && at(CN + 2) == 0x00 &&
             at(CN + 16) == 0x50 && at(CN + 17) == 0x55 && at(CN + 18) == 0x05;
    /* [5] */
    l = font_load("font", 4, 0x44000UL);
    ok = l && l->height == 5 && l->first == 32 && font_glyph_width(l, 'A') == 4 && l->glyphs < 0x45000UL;
    l = font_load("fpack", 5, 0x44800UL);
    ok = ok && l && l->height == 5 && font_glyph_width(l, 'I') == 1 && l->glyphs > 0x46000UL && l->glyphs < 0x47000UL;
    l = font_load("nofont", 6, 0x44000UL);
    OUT[5] = ok && !l && load_error == 4;
    OUT[6] = 0xAA;
    return 0;
}
