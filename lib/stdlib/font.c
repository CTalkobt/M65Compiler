/* font.c — <font.h> (#466): font45's fonts read, measured and drawn. */
#include <font.h>
#include <dma.h>

static unsigned char ramp_first = 1;
static signed int clip_x0 = 0, clip_y0 = 0, clip_x1 = 32767, clip_y1 = 32767;   /* (nothing left of or above the buffer) */

const font_t *font_from(font_t *f, unsigned long data) {
    const unsigned char __far *p = (const unsigned char __far *)data;
    if (p[0] != 0x46 || p[1] != 0x34 || p[2] != 1) return 0;   /* "F4" (ASCII), version 1 */
    f->height = p[3];
    f->baseline = p[4];
    f->first = p[5];
    f->last = p[6];
    f->bpp = p[7];
    f->flags = p[8];
    f->cellw = p[9];
    f->rowbytes = p[10];
    if (f->last < f->first || !f->height || (f->bpp != 1 && f->bpp != 2 && f->bpp != 4)) return 0;
    if (f->flags & FONT_PROPORTIONAL) {
        f->widths = data + 12;
        f->glyphs = data + 12 + (unsigned int)(f->last - f->first) + 1;
    } else {
        f->widths = 0;
        f->glyphs = data + 12;
    }
    return f;
}

void font_install(unsigned long charset, unsigned int bytes, unsigned long lin) {
    dma_copy_far(lin, charset, bytes);
}

void font_select(unsigned long lin) {
    *(volatile unsigned char *)0xD068 = (unsigned char)lin;
    *(volatile unsigned char *)0xD069 = (unsigned char)(lin >> 8);
    *(volatile unsigned char *)0xD06A = (unsigned char)(lin >> 16);
}

unsigned char font_glyph_width(const font_t *f, unsigned char c) {
    if (c < f->first || c > f->last) return 0;
    if (!f->widths) return f->cellw;
    return ((const unsigned char __far *)f->widths)[c - f->first];
}

unsigned int font_width(const font_t *f, const char *s) {
    unsigned int w = 0;
    while (*s) w += font_glyph_width(f, (unsigned char)*s++);
    return w;
}

void font_ramp(unsigned char first, unsigned char levels) {
    (void)levels;               /* (level n is first + n - 1: the count is the font's) */
    ramp_first = first;
}

void font_clip(signed int x0, signed int y0, signed int x1, signed int y1) {
    clip_x0 = x0; clip_y0 = y0; clip_x1 = x1; clip_y1 = y1;
}

signed int font_draw_bitmap(const font_t *f, unsigned long lin, unsigned int pitch, signed int x, signed int y, const char *s) {
    unsigned char bpp = f->bpp, mask = (unsigned char)((1 << bpp) - 1);
    unsigned int glyphBytes = (unsigned int)f->height * f->rowbytes;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        unsigned char w = font_glyph_width(f, c), row, col;
        if (c >= f->first && c <= f->last) {
            const unsigned char __far *g =
                (const unsigned char __far *)(f->glyphs + (unsigned long)(c - f->first) * glyphBytes);
            for (row = 0; row < f->height; row++) {
                signed int py = y + row;
                const unsigned char __far *r = g + (unsigned int)row * f->rowbytes;
                if (py < clip_y0 || py > clip_y1) continue;
                for (col = 0; col < f->cellw; col++) {     /* (the whole cell: an italic glyph reaches past its advance) */
                    signed int px = x + col;
                    unsigned int bit = (unsigned int)col * bpp;
                    unsigned char v = (unsigned char)(r[bit >> 3] >> (8 - bpp - (bit & 7))) & mask;
                    if (!v || px < clip_x0 || px > clip_x1) continue;
                    *(unsigned char __far *)(lin + (unsigned long)py * pitch + (unsigned int)px) =
                        (unsigned char)(ramp_first + v - 1);
                }
            }
        }
        x += w;
    }
    return x;
}
