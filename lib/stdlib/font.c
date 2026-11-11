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
    f->cols = f->rows = 0;
    if (f->last < f->first || !f->height) return 0;
    if (f->flags & (FONT_FCM | FONT_NCM)) {
        /* a glyph set: its characters from the next multiple of 64, the font itself 64-byte aligned */
        unsigned int head = 12;
        if ((f->flags & FONT_FCM) ? f->bpp != 8 : f->bpp != 4) return 0;
        if ((unsigned char)data & 63) return 0;
        f->cols = p[10];
        f->rows = p[11];
        if (f->flags & FONT_PROPORTIONAL) head += (unsigned int)(f->last - f->first) + 1;
        f->widths = (f->flags & FONT_PROPORTIONAL) ? data + 12 : 0;
        f->glyphs = data + ((head + 63) & ~63);
        return f;
    }
    if (f->bpp != 1 && f->bpp != 2 && f->bpp != 4) return 0;
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

/* a glyph's rows (columns c0..c1 - 1 of each) into a linear buffer (font_glyph.s45) */
__fastcall__ void __font_glyph(unsigned long src, unsigned long dst, unsigned char c0, unsigned char c1, unsigned char bpp,
                               unsigned char ink0, unsigned char rows, unsigned char rowbytes, unsigned int pitch);

/* a string's pixels, plotted into a linear buffer (kind 0: lin + y * pitch + x),
 * full-colour characters (1) or nibble-colour ones (2: a nibble a pixel) */
static signed int draw(const font_t *f, unsigned char kind, unsigned long lin, signed int pitch, signed int x, signed int y,
                       const char *s) {
    unsigned char bpp = f->bpp, mask = (unsigned char)((1 << bpp) - 1);
    unsigned int glyphBytes = (unsigned int)f->height * f->rowbytes;
    if (f->flags & (FONT_FCM | FONT_NCM)) return x;      /* (a glyph set: font_put_cells) */
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        unsigned char w = font_glyph_width(f, c), row, col;
        if (c >= f->first && c <= f->last && kind == 0) {
            /* a linear buffer: the glyph's visible rows and columns, drawn in assembly
             * (in longs: the default clip, 32767, minus x overflows an int) */
            signed long c0 = (signed long)clip_x0 - x, c1 = (signed long)clip_x1 - x + 1;
            signed long r0 = (signed long)clip_y0 - y, r1 = (signed long)clip_y1 - y + 1;
            if (c0 < 0) c0 = 0;
            if (c1 > f->cellw) c1 = f->cellw;
            if (r0 < 0) r0 = 0;
            if (r1 > f->height) r1 = f->height;
            if (c0 < c1 && r0 < r1)
                __font_glyph(f->glyphs + (unsigned long)(c - f->first) * glyphBytes + (unsigned int)r0 * f->rowbytes,
                             lin + (unsigned long)(y + r0) * (unsigned int)pitch + (unsigned long)(signed long)x,
                             (unsigned char)c0, (unsigned char)c1, bpp, (unsigned char)(ramp_first - 1),
                             (unsigned char)(r1 - r0), f->rowbytes, (unsigned int)pitch);
        } else if (c >= f->first && c <= f->last) {
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
                    unsigned char ink;
                    unsigned long a;
                    unsigned int ch;
                    if (!v || px < clip_x0 || px > clip_x1) continue;
                    ink = (unsigned char)(ramp_first + v - 1);
                    /* the character: down the columns (pitch > 0 characters a column) or along the rows */
                    if (kind == 1) ch = pitch > 0 ? (unsigned int)(px >> 3) * (unsigned int)pitch + (unsigned int)(py >> 3)
                                                  : (unsigned int)(py >> 3) * (unsigned int)-pitch + (unsigned int)(px >> 3);
                    else ch = pitch > 0 ? (unsigned int)(px >> 4) * (unsigned int)pitch + (unsigned int)(py >> 3)
                                        : (unsigned int)(py >> 3) * (unsigned int)-pitch + (unsigned int)(px >> 4);
                    a = lin + (unsigned long)ch * 64 + (unsigned int)((py & 7) << 3);
                    if (kind == 1) {
                        *(unsigned char __far *)(a + (unsigned int)(px & 7)) = ink;
                    } else {
                        unsigned char __far *b = (unsigned char __far *)(a + (unsigned int)((px & 15) >> 1));
                        *b = (px & 1) ? (unsigned char)((*b & 0x0F) | (ink << 4)) : (unsigned char)((*b & 0xF0) | (ink & 0x0F));
                    }
                }
            }
        }
        x += w;
    }
    return x;
}

signed int font_draw_bitmap(const font_t *f, unsigned long lin, unsigned int pitch, signed int x, signed int y, const char *s) {
    return draw(f, 0, lin, (signed int)pitch, x, y, s);
}

signed int font_draw_fcm(const font_t *f, unsigned long chars, signed int strip, signed int x, signed int y, const char *s) {
    return draw(f, 1, chars, strip, x, y, s);
}

signed int font_draw_ncm(const font_t *f, unsigned long chars, signed int strip, signed int x, signed int y, const char *s) {
    return draw(f, 2, chars, strip, x, y, s);
}

unsigned char font_put_cells(const font_t *f, unsigned long screen, unsigned int row_bytes, unsigned char col, unsigned char row,
                             const char *s) {
    unsigned char cw = (f->flags & FONT_NCM) ? 16 : 8;
    unsigned int per = (unsigned int)f->cols * f->rows;
    if (!(f->flags & (FONT_FCM | FONT_NCM))) return col;
    while (*s) {
        unsigned char c = (unsigned char)*s++, cx, cy, across;
        unsigned int first;
        if (c < f->first || c > f->last) continue;
        /* the glyph's first character's number (the set's characters: glyphs / 64 on) */
        first = (unsigned int)(f->glyphs >> 6) + (unsigned int)(c - f->first) * per;
        across = f->cols;
        if (f->widths) {
            /* proportional: the characters its width covers */
            unsigned char w = ((const unsigned char __far *)f->widths)[c - f->first];
            across = (unsigned char)((w + cw - 1) / cw);
            if (across > f->cols) across = f->cols;
        }
        for (cx = 0; cx < across; cx++)
            for (cy = 0; cy < f->rows; cy++) {
                unsigned int n = first + (unsigned int)cx * f->rows + cy;
                unsigned char __far *e =
                    (unsigned char __far *)(screen + (unsigned long)(row + cy) * row_bytes + (unsigned int)(col + cx) * 2);
                e[0] = (unsigned char)n;
                e[1] = (unsigned char)(n >> 8);
            }
        col += across;
    }
    return col;
}
