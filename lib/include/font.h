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

typedef struct {
    unsigned char height, baseline, first, last, bpp, flags;
    unsigned char cellw, rowbytes;          /* the cell's width, a glyph row's bytes */
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

#endif
