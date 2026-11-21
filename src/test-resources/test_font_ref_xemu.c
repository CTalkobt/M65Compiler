/* test_font_ref_xemu.c — <font.h> in assembly (font.s45) against the C it
 * replaced (here as ref_*: font.c's code, the linear buffer a pixel at a
 * time), with fonts font45 makes (test_xemu.sh, build/test/fontr):
 *   tiny.c   tiny.txt, 1 bpp proportional (A at 193, PETSCII)
 *   seg2.c   7-segment, 16 rows, 2 bpp fixed, the digits
 *   seg4.c   the same at 4 bpp
 *   segf.c   the same as FCM characters, 2 x 2 a glyph
 *   tinyf.c  tiny.txt as a proportional FCM glyph set (A to I)
 *   tinyn.c  tiny.txt as a proportional NCM glyph set
 *
 * Results at $C0C0 (01 = pass):
 *   [0] font_from: the same font_t and result for each font, and for copies
 *       with a bad header, version, bpp, first > last, height 0, a glyph
 *       set off its 64-byte alignment
 *   [1] font_glyph_width for every code, font_width of strings
 *   [2] font_draw_bitmap: the same pixels and result for 3 fonts, x and y
 *       at and past each edge, 4 clips (negative, empty), ramps
 *   [3] font_draw_fcm: strips 4 and -5, the same cases (clips >= 0); and
 *       pixels at negative x or y (clips < 0), the character numbers kept
 *       in the buffer (-1 cells, -1 rows of characters: 16-bit sums)
 *   [4] font_draw_ncm: strips 3 and -4, ramps reaching past entry 15; the
 *       same negative cases
 *   [5] font_put_cells: the sets, rows and columns, width tables made up
 *       (0 to 255 pixels: 1 up to all of a glyph's columns)
 *   [6] $AA sentinel
 */
#include <font.h>
#include <string.h>
#include "tiny.h"
#include "seg2.h"
#include "seg4.h"
#include "segf.h"
#include "tinyf.h"
#include "tinyn.h"

#define OUT ((volatile unsigned char *)0xC0C0)
#define LIN(p) ((unsigned long)(unsigned int)(p))
#define MARGIN 320
#define SIZE 2304

static unsigned char bufa[SIZE], bufb[SIZE];

/* ---- the reference: font.c ---- */
static unsigned char ref_ramp_first = 1;
static signed int ref_x0 = 0, ref_y0 = 0, ref_x1 = 32767, ref_y1 = 32767;

static const font_t *ref_from(font_t *f, unsigned long data) {
    const unsigned char __far *p = (const unsigned char __far *)data;
    if (p[0] != 0x46 || p[1] != 0x34 || p[2] != 1) return 0;
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

static unsigned char ref_gw(const font_t *f, unsigned char c) {
    if (c < f->first || c > f->last) return 0;
    if (!f->widths) return f->cellw;
    return ((const unsigned char __far *)f->widths)[c - f->first];
}

static unsigned int ref_width(const font_t *f, const char *s) {
    unsigned int w = 0;
    while (*s) w += ref_gw(f, (unsigned char)*s++);
    return w;
}

static signed int ref_draw(const font_t *f, unsigned char kind, unsigned long lin, signed int pitch, signed int x, signed int y,
                           const char *s) {
    unsigned char bpp = f->bpp, mask = (unsigned char)((1 << bpp) - 1);
    unsigned int glyphBytes = (unsigned int)f->height * f->rowbytes;
    if (f->flags & (FONT_FCM | FONT_NCM)) return x;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        unsigned char w = ref_gw(f, c), row, col;
        if (c >= f->first && c <= f->last) {
            const unsigned char __far *g =
                (const unsigned char __far *)(f->glyphs + (unsigned long)(c - f->first) * glyphBytes);
            for (row = 0; row < f->height; row++) {
                signed int py = y + row;
                const unsigned char __far *r = g + (unsigned int)row * f->rowbytes;
                if (py < ref_y0 || py > ref_y1) continue;
                for (col = 0; col < f->cellw; col++) {
                    signed int px = x + col;
                    unsigned int bit = (unsigned int)col * bpp;
                    unsigned char v = (unsigned char)(r[bit >> 3] >> (8 - bpp - (bit & 7))) & mask;
                    unsigned char ink;
                    unsigned long a;
                    unsigned int ch;
                    if (!v || px < ref_x0 || px > ref_x1) continue;
                    ink = (unsigned char)(ref_ramp_first + v - 1);
                    if (kind == 0) {
                        *(unsigned char __far *)(lin + (unsigned long)(signed long)py * (unsigned int)pitch + (unsigned long)(signed long)px) = ink;
                        continue;
                    }
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

static unsigned char ref_put_cells(const font_t *f, unsigned long screen, unsigned int row_bytes, unsigned char col, unsigned char row,
                                   const char *s) {
    unsigned char cw = (f->flags & FONT_NCM) ? 16 : 8;
    unsigned int per = (unsigned int)f->cols * f->rows;
    if (!(f->flags & (FONT_FCM | FONT_NCM))) return col;
    while (*s) {
        unsigned char c = (unsigned char)*s++, cx, cy, across;
        unsigned int first;
        if (c < f->first || c > f->last) continue;
        first = (unsigned int)(f->glyphs >> 6) + (unsigned int)(c - f->first) * per;
        across = f->cols;
        if (f->widths) {
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

/* ---- the cases ---- */
static font_t fa, fb;
static unsigned char copy[64 + 80];

static unsigned char same_from(unsigned long data) {
    const font_t *a, *b;
    memset(&fa, 0x5A, sizeof fa);
    memset(&fb, 0x5A, sizeof fb);
    a = font_from(&fa, data);
    b = ref_from(&fb, data);
    return (a == 0) == (b == 0) && (!a || a == &fa) && !memcmp(&fa, &fb, sizeof fa);
}

/* a copy of a font's first 80 bytes, 64-byte aligned (+ off), one byte changed */
static unsigned long changed(const unsigned char *font, unsigned char off, unsigned char at, unsigned char v) {
    unsigned char *p = (unsigned char *)(((unsigned int)copy + 63) & ~63u) + off;
    memcpy(p, font, 80);
    if (at < 80) p[at] = v;
    return LIN(p);
}

static void clip(signed int x0, signed int y0, signed int x1, signed int y1) {
    font_clip(x0, y0, x1, y1);
    ref_x0 = x0; ref_y0 = y0; ref_x1 = x1; ref_y1 = y1;
}

static void ramp(unsigned char first) {
    font_ramp(first, 3);
    ref_ramp_first = first;
}

static const signed int xs[] = { -9, -3, 0, 5, 13 };
static const signed int ys[] = { -7, 0, 6 };
static const char *strs[] = { "AIB", "0123", "4x567", "89" };

/* each font, position and clip; kind 0 bitmap (pitch 32), 1 FCM, 2 NCM */
static unsigned char draws(unsigned char kind, signed int pitch) {
    static const signed int cl[4][4] = {
        { 0, 0, 32767, 32767 }, { 2, 1, 20, 14 }, { -5, -5, 8, 6 }, { 10, 10, 9, 9 } };
    static const signed int clc[3][4] = { { 0, 0, 32767, 32767 }, { 3, 2, 40, 9 }, { 1, 9, 60, 23 } };
    font_t *fs[3];
    font_t f1, f2, f3;
    const char *ss[3];
    unsigned char fi, xi, yi, ci, nclips, ok = 1, n = 0;
    unsigned long la = LIN(bufa) + (kind ? 0 : MARGIN), lb = LIN(bufb) + (kind ? 0 : MARGIN);
    font_from(&f1, LIN(tiny));
    font_from(&f2, LIN(seg2));
    font_from(&f3, LIN(seg4));
    fs[0] = &f1; fs[1] = &f2; fs[2] = &f3;
    ss[0] = strs[0]; ss[1] = strs[1]; ss[2] = strs[2];
    nclips = kind ? 3 : 4;
    for (fi = 0; fi < 3; fi++)
        for (ci = 0; ci < nclips; ci++) {
            const signed int *c = kind ? clc[ci] : cl[ci];
            clip(c[0], c[1], c[2], c[3]);
            for (xi = 0; xi < 5; xi++)
                for (yi = 0; yi < 3; yi++) {
                    signed int ra, rb;
                    signed int x = xs[xi], y = ys[yi];
                    if (kind && (x < 0 || y < 0)) { x += 9; y += 7; }
                    ramp((unsigned char)(1 + (n++ % 7) * 3));
                    memset(bufa, 0x11, SIZE);
                    memset(bufb, 0x11, SIZE);
                    if (kind == 0) {
                        ra = font_draw_bitmap(fs[fi], la, (unsigned int)pitch, x, y, ss[fi]);
                    } else if (kind == 1) {
                        ra = font_draw_fcm(fs[fi], la, pitch, x, y, ss[fi]);
                    } else {
                        ra = font_draw_ncm(fs[fi], la, pitch, x, y, ss[fi]);
                    }
                    rb = ref_draw(fs[fi], kind, lb, pitch, x, y, ss[fi]);
                    if (ra != rb || memcmp(bufa, bufb, SIZE)) ok = 0;
                }
        }
    clip(0, 0, 32767, 32767);
    ramp(1);
    return ok;
}

/* pixels left of or above 0 (#670): strip < 0, x from -9 and y from 8 (the
 * character (py >> 3) * -strip + (px >> 3), a cell -1 or -2); strip > 0, y
 * from -7 and x from 16 ((px >> 3) * strip + (py >> 3), a row -1) */
static unsigned char draws_neg(unsigned char kind, signed int pitch) {
    static const signed int xn[] = { -9, -3, -1 }, yn[] = { 8, 9 };
    static const signed int xp[] = { 16, 21 }, yp[] = { -7, -3 };
    font_t f1, f2, f3;
    font_t *fs[3];
    unsigned char fi, xi, yi, ok = 1;
    font_from(&f1, LIN(tiny));
    font_from(&f2, LIN(seg2));
    font_from(&f3, LIN(seg4));
    fs[0] = &f1; fs[1] = &f2; fs[2] = &f3;
    if (pitch < 0) clip(-12, -10, 40, 30);
    else clip(-12, -10, 60, 30);
    for (fi = 0; fi < 3; fi++)
        for (xi = 0; xi < (pitch < 0 ? 3 : 2); xi++)
            for (yi = 0; yi < 2; yi++) {
                signed int ra, rb, x = pitch < 0 ? xn[xi] : xp[xi], y = pitch < 0 ? yn[yi] : yp[yi];
                ramp((unsigned char)(2 + xi * 5 + yi));
                memset(bufa, 0x11, SIZE);
                memset(bufb, 0x11, SIZE);
                ra = kind == 1 ? font_draw_fcm(fs[fi], LIN(bufa), pitch, x, y, strs[fi])
                               : font_draw_ncm(fs[fi], LIN(bufa), pitch, x, y, strs[fi]);
                rb = ref_draw(fs[fi], kind, LIN(bufb), pitch, x, y, strs[fi]);
                if (ra != rb || memcmp(bufa, bufb, SIZE)) ok = 0;
            }
    clip(0, 0, 32767, 32767);
    ramp(1);
    return ok;
}

int main(void) {
    font_t f, s;
    unsigned char ok, i;
    unsigned int c;
    static unsigned char wt[16];
    static const unsigned char wv[16] = { 0, 1, 7, 8, 9, 15, 16, 17, 24, 31, 32, 33, 128, 250, 254, 255 };
    /* [0] */
    ok = same_from(LIN(tiny)) && same_from(LIN(seg2)) && same_from(LIN(seg4)) && same_from(LIN(segf)) &&
         same_from(LIN(tinyf)) && same_from(LIN(tinyn)) && same_from(LIN(segf) + 1);
    ok = ok && same_from(changed(tiny, 0, 0, 'G')) && same_from(changed(tiny, 0, 1, '5')) && same_from(changed(tiny, 0, 2, 2)) &&
         same_from(changed(tiny, 0, 7, 3)) && same_from(changed(tiny, 0, 7, 8)) && same_from(changed(tiny, 0, 5, 202)) &&
         same_from(changed(tiny, 0, 3, 0)) && same_from(changed(tiny, 0, 99, 0)) && same_from(changed(tiny, 5, 99, 0)) &&
         same_from(changed(seg2, 3, 99, 0)) && same_from(changed(segf, 0, 99, 0)) && same_from(changed(segf, 0, 7, 4)) &&
         same_from(changed(segf, 0, 8, 6)) && same_from(changed(segf, 0, 8, 4)) && same_from(changed(segf, 32, 99, 0)) &&
         same_from(changed(tinyn, 0, 99, 0)) && same_from(changed(tinyn, 0, 7, 8)) && same_from(changed(tinyf, 0, 99, 0)) &&
         same_from(changed(tinyf, 0, 8, 3)) && same_from(changed(tiny, 0, 8, 0));
    OUT[0] = ok;
    /* [1] */
    ok = 1;
    font_from(&f, LIN(tiny));
    font_from(&s, LIN(seg4));
    for (c = 0; c < 256; c++)
        if (font_glyph_width(&f, (unsigned char)c) != ref_gw(&f, (unsigned char)c) ||
            font_glyph_width(&s, (unsigned char)c) != ref_gw(&s, (unsigned char)c)) ok = 0;
    font_from(&s, LIN(tinyf));
    for (c = 0; c < 256; c++) if (font_glyph_width(&s, (unsigned char)c) != ref_gw(&s, (unsigned char)c)) ok = 0;
    if (font_width(&f, "") != 0 || font_width(&f, "AB I") != ref_width(&f, "AB I") || font_width(&f, "AB I") != 11 ||
        font_width(&f, "ZZAZ") != ref_width(&f, "ZZAZ"))
        ok = 0;
    font_from(&s, LIN(seg4));
    if (font_width(&s, "0123456789 x") != ref_width(&s, "0123456789 x")) ok = 0;
    OUT[1] = ok;
    /* [2] - [4] */
    OUT[2] = draws(0, 32);
    OUT[3] = draws(1, 4) && draws(1, -5) && draws_neg(1, 4) && draws_neg(1, -5);
    OUT[4] = draws(2, 3) && draws(2, -4) && draws_neg(2, 3) && draws_neg(2, -4);
    /* [5] */
    ok = 1;
    for (i = 0; i < 3; i++) {
        font_t g;
        unsigned char k, ra, rb;
        font_from(&g, i == 0 ? LIN(segf) : i == 1 ? LIN(tinyf) : LIN(tinyn));
        for (k = 0; k < 4; k++) {
            unsigned int rbytes = k & 1 ? 80 : 62;
            memset(bufa, 0x22, SIZE);
            memset(bufb, 0x22, SIZE);
            ra = font_put_cells(&g, LIN(bufa), rbytes, k * 3, k, i == 0 ? "0189x5" : "AxBI ");
            rb = ref_put_cells(&g, LIN(bufb), rbytes, k * 3, k, i == 0 ? "0189x5" : "AxBI ");
            if (ra != rb || memcmp(bufa, bufb, SIZE)) ok = 0;
        }
    }
    /* made-up width tables on the 2 x 2 set: 1 or 2 columns, never more */
    font_from(&f, LIN(segf));
    memcpy(wt, wv, 16);
    f.widths = LIN(wt);
    f.first = '0';
    f.last = '0' + 15;
    for (i = 0; i < 2; i++) {
        unsigned char ra, rb;
        static const char all[] = "0123456789:;<=>?";
        if (i) f.flags = (f.flags & ~FONT_FCM) | FONT_NCM;   /* 16 pixels a character */
        memset(bufa, 0x33, SIZE);
        memset(bufb, 0x33, SIZE);
        ra = font_put_cells(&f, LIN(bufa), 128, 1, 2, all);
        rb = ref_put_cells(&f, LIN(bufb), 128, 1, 2, all);
        if (ra != rb || memcmp(bufa, bufb, SIZE)) ok = 0;
    }
    /* not a glyph set: col back, nothing written */
    font_from(&f, LIN(tiny));
    memset(bufa, 0x44, SIZE);
    if (font_put_cells(&f, LIN(bufa), 80, 7, 1, "AB") != 7 || bufa[0] != 0x44 || bufa[2] != 0x44) ok = 0;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
