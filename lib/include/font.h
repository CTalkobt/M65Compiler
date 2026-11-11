/* font.h — fonts font45 makes, drawn on the MEGA65 (#466)
 *
 * font45 convert ... --bpp N [--proportional] writes a font (format 1, as C,
 * assembly or a binary file); font45 convert ... --charset ORDER an 8x8
 * charset for the VIC-IV's text mode.
 *
 *   font_from     a font_t for font45's data at a 28-bit address (a C array:
 *                 (unsigned long)(unsigned int)name; a file font45 --bin wrote,
 *                 loaded anywhere); NULL if it is not one
 *   font_install  a charset's bytes copied to chip RAM at lin (DMA)
 *   font_select   the VIC-IV's character set: lin ($D068-$D06A)
 *   font_glyph_width / font_width   a code's / a string's width in pixels
 *   font_ramp     the palette entries a glyph's coverage levels are drawn
 *                 in: level n (1..) is first + n - 1 (default: 1 bpp level
 *                 1 is entry 1)
 *   font_clip     the rectangle drawing keeps to (inclusive; default: x, y >= 0)
 *   font_draw_bitmap  a string drawn into a linear 8-bit-a-pixel buffer at
 *                 lin, pitch bytes a row, its top-left at x, y (the glyphs'
 *                 cell top, not the baseline); coverage 0 is not drawn. It
 *                 returns the x after the last glyph.
 *   font_draw_fcm / font_draw_ncm  the same into the VIC-IV's full-colour
 *                 (8x8, a byte a pixel) / nibble-colour (16x8, the left pixel
 *                 the low nibble) characters at chars: numbered down the
 *                 columns, strip characters a column (strip > 0), or along
 *                 the rows, -strip a row (strip < 0); x, y in pixels
 *   font_put_cells  a glyph set's (font45 --fcm / --ncm) characters put on a
 *                 16-bit character screen (2 bytes a character, CHR16) at
 *                 screen, row_bytes a row, from column col, row row: each
 *                 glyph its cols x rows characters; the column after. The
 *                 set must be at a multiple of 64 (font45's C array is),
 *                 where the VIC-IV sees it; colour RAM is the program's
 *                 (NCM: byte 0 bit 3 a character)
 *   font_load     a font file font45 wrote (--bin; --pack too) loaded from
 *                 the program's drive to lin (<load.h>) and unpacked if
 *                 packed (to the address it was packed for): a font_t for it
 *                 (one, overwritten by the next font_load); NULL on an error
 *                 (load_error says which; 0: not a font)
 *
 *   #include <font.h>
 *   #include "title.h"                 font45 convert title.bdf --bpp 2 ... -o title.c
 *   font_t t;
 *   font_from(&t, (unsigned long)(unsigned int)title);
 *   font_ramp(16, 3);                  levels 1-3: palette entries 16-18
 *   font_draw_bitmap(&t, 0x40000UL, 320, 10, 20, "Hello");
 */
#ifndef FONT_H
#define FONT_H

#define FONT_PROPORTIONAL 1     /* flags: a width per glyph */
#define FONT_FCM 2              /* a glyph set of full-colour characters (bpp 8) */
#define FONT_NCM 4              /* a glyph set of nibble-colour characters (bpp 4) */

typedef struct {
    unsigned char height, baseline, first, last, bpp, flags;
    unsigned char cellw, rowbytes;          /* the cell's width, a glyph row's bytes */
    unsigned char cols, rows;               /* a glyph set's characters across and down a glyph */
    unsigned long widths;                   /* the width table (0: fixed, cellw) */
    unsigned long glyphs;                   /* the first glyph */
} font_t;

const font_t *font_from(font_t *f, unsigned long data);

void font_install(unsigned long charset, unsigned int bytes, unsigned long lin);
void font_select(unsigned long lin);

unsigned char font_glyph_width(const font_t *f, unsigned char c);
unsigned int font_width(const font_t *f, const char *s);

void font_ramp(unsigned char first, unsigned char levels);
void font_clip(signed int x0, signed int y0, signed int x1, signed int y1);
signed int font_draw_bitmap(const font_t *f, unsigned long lin, unsigned int pitch, signed int x, signed int y, const char *s);
signed int font_draw_fcm(const font_t *f, unsigned long chars, signed int strip, signed int x, signed int y, const char *s);
signed int font_draw_ncm(const font_t *f, unsigned long chars, signed int strip, signed int x, signed int y, const char *s);

unsigned char font_put_cells(const font_t *f, unsigned long screen, unsigned int row_bytes, unsigned char col, unsigned char row,
                             const char *s);

const font_t *font_load(const char *name, unsigned char len, unsigned long lin);

#endif
