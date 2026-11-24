/* tgi_gendraw.c — <tgi.h>'s generic drivers' drawing (tgi_gen.s45 has their
 * dispatch; each driver its own pixels): lines, glyphs and bars a pixel at a
 * time through tgi_setpixel; the text drivers' text into the screen's
 * cells; tgi_toggle_case; tgi_copyrect (any driver)
 *
 * Linked with a generic driver (tgi_gen.s45 refers to __tgi_gen_link), its
 * __tgi_g_* replacing tgi_pixel.s45's and tgi.c's weak ones.
 */

/* Written for standard C signedness (coordinates) */
#pragma cc45 signed_default

#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <dma.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))

extern unsigned char __tgi_mode;

const unsigned char __tgi_gen_link = 0;

/* a line with both ends on the screen (tgi_pixel.s45's __tgi_line_on_screen) */
__fastcall__ void __tgi_g_line(int x1, int y1, int x2, int y2) {
    int dx = x2 > x1 ? x2 - x1 : x1 - x2, sx = x1 < x2 ? 1 : -1;
    int dy = y2 > y1 ? y1 - y2 : y2 - y1, sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    for (;;) {
        tgi_setpixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

/* an 8x8 glyph (8 bytes at the 28-bit address) at (x, y), its set pixels */
__fastcall__ void __tgi_g_glyph(int x, int y, unsigned long glyph) {
    unsigned char rows[8], r, b, bits;
    dma_copy_far((unsigned long)(unsigned int)rows, glyph, 8);
    for (r = 0; r < 8; r++) {
        bits = rows[r];
        for (b = 0; b < 8; b++, bits <<= 1)
            if (bits & 0x80) tgi_setpixel(x + b, y + r);
    }
}

/* a bar (clipped, x1 <= x2, y1 <= y2) in the current colour */
__fastcall__ void __tgi_g_bar(int x1, int y1, int x2, int y2) {
    int x, y;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) tgi_setpixel(x, y);
}

/* ===== The text drivers' text: characters into the cells ===== */

/* PETSCII to a screen code */
static unsigned char scrcode(unsigned char c) {
    if (c < 0x20) return 0xFF;                  /* (a control character: none) */
    if (c < 0x40) return c;
    if (c < 0x60) return (unsigned char)(c - 0x40);
    if (c < 0x80) return (unsigned char)(c - 0x20);
    if (c < 0xA0) return 0xFF;
    if (c < 0xC0) return (unsigned char)(c - 0x40);
    if (c == 0xFF) return 0x5E;
    return (unsigned char)(c - 0x80);
}

/* cell (col, row) := screen code sc in the current colour */
void __tgi_t_cell(unsigned int col, unsigned int row, unsigned char sc) {
    unsigned long scr = REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16) |
                        ((unsigned long)(REG(0xD063) & 0x0F) << 24);
    unsigned int step = REG(0xD058) | ((unsigned int)REG(0xD059) << 8);
    unsigned int off = row * step + col;
    unsigned char colr = (unsigned char)(tgi_color & 0x0F);
    unsigned long colram = 0xFF80000UL + (REG(0xD064) | ((unsigned int)REG(0xD065) << 8));
    dma_copy_far(scr + off, (unsigned long)(unsigned int)&sc, 1);
    dma_copy_far(colram + off, (unsigned long)(unsigned int)&colr, 1);
}

/* tgi_outtext on a text driver with the bitmap font: each character into
 * the cell at the cursor (a cell is 2x2 of the driver's pixels), the cursor
 * moved on a cell (horizontal: right; vertical: up) */
__fastcall__ void __tgi_t_outtext(const char *s) {
    unsigned char c, sc;
    int cols = (int)((tgi_xmax + 1) >> 1), rows = (int)((tgi_ymax + 1) >> 1);
    while ((c = (unsigned char)*s++) != 0) {
        int cx = tgi_curx >> 1, cy = tgi_cury >> 1;
        sc = scrcode(c);
        if (sc != 0xFF && cx >= 0 && cy >= 0 && cx < cols && cy < rows)
            __tgi_t_cell((unsigned int)cx, (unsigned int)cy, sc);
        if (tgi_textdir == TGI_TEXT_VERTICAL) tgi_cury -= 2;
        else tgi_curx += 2;
    }
}

/* The character set: the upper-case/graphics one or the lower/upper-case
 * one (CHR$(142) / CHR$(14) to the screen editor: $D069 bit 3 then); 1 now
 * lower case */
unsigned char tgi_toggle_case(void) {
    unsigned char lower = (REG(0xD069) & 0x08) != 0;
    putchar(lower ? 142 : 14);
    return !lower;
}

/* ===== tgi_copyrect, for any driver ===== */

/* the rectangle (x1, y1)-(x2, y2) to (dx, dy), pixel by pixel through
 * tgi_getpixel / tgi_setpixel (off the screen: 0 read, nothing written), in
 * the order that keeps an overlapping copy right */
void tgi_copyrect(int x1, int y1, int x2, int y2, int dx, int dy) {
    unsigned char keep = tgi_color;
    int t, w, h, i, j, ix, iy;
    if (!tgi_drv) return;
    if (x1 > x2) { t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; }
    w = x2 - x1;
    h = y2 - y1;
    for (j = 0; j <= h; j++) {
        iy = dy > y1 ? h - j : j;
        for (i = 0; i <= w; i++) {
            ix = dx > x1 ? w - i : i;
            tgi_color = tgi_getpixel(x1 + ix, y1 + iy);
            tgi_setpixel(dx + ix, dy + iy);
        }
    }
    tgi_color = keep;
}
