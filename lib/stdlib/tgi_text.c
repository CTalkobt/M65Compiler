/* tgi_text.c — <tgi.h>'s text: the bitmap font and vector fonts
 *
 * Bitmap text is the lower/upper case half of the VIC's current font
 * (CHARPTR; the character ROM for the VIC-II's $1000/$1800/$9000/$9800),
 * 8x8, so a PETSCII string shows in both cases whichever half the screen
 * shows (control characters take their place but draw nothing),
 * magnified by the 8.8 scales rounded to whole numbers (tgi_flags bit 7 is
 * clear). The cursor is the text's top left; TGI_TEXT_VERTICAL turns it
 * anticlockwise: the cursor at its bottom left, the text going up.
 *
 * Vector text is as cc65's tgi_vectorchar: a character's points are
 * offsets from the cursor (on the baseline, Y up) times the 8.8 scales;
 * TGI_TEXT_VERTICAL turns it anticlockwise. The string is converted from
 * PETSCII to ASCII.
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <tgi/tgi-vectorfont.h>
#include <string.h>
#include "tgi_drv.h"

#define REG(a) (*(volatile unsigned char *)(a))

/* ============================================================================
 * Style
 * ========================================================================== */

/* One dimension: the vector font's scale, the bitmap font's (rounded to a
 * whole number unless the driver scales finely), the bitmap character's size
 * in pixels (8.8) */
static void scale(unsigned s, unsigned *sc, unsigned *size, unsigned char px) {
    unsigned b = s;
    sc[0] = s;
    if (!(tgi_flags & 0x80)) b = (s + 0x80u) & 0xFF00u;
    sc[1] = b;
    *size = (unsigned)((unsigned long)px * b);
}

void tgi_settextscale(unsigned width, unsigned height) {
    scale(width, tgi_textscalew, &tgi_charwidth, tgi_fontwidth);
    scale(height, tgi_textscaleh, &tgi_charheight, tgi_fontheight);
}

void tgi_settextdir(unsigned char dir) {
    tgi_textdir = dir;
}

void tgi_settextstyle(unsigned width, unsigned height, unsigned char dir, unsigned char font) {
    tgi_font = font;
    tgi_textdir = dir;
    tgi_settextscale(width, height);
}

void tgi_install_vectorfont(const tgi_vectorfont *font) {
    __tgi_vfont = font;
}

/* ============================================================================
 * Sizes
 * ========================================================================== */

/* PETSCII c as ASCII (as cc65's toascii for the CBM machines) */
static unsigned char ascii(unsigned char c) {
    if (c >= 0x41 && c <= 0x5A) return (unsigned char)(c + 0x20);
    if (c >= 0xC1 && c <= 0xDA) return (unsigned char)(c - 0x80);
    return c;
}

/* A vector font character's width (0: not in the font) */
static unsigned char vwidth(unsigned char a) {
    if (a < TGI_VF_FIRSTCHAR || a > TGI_VF_LASTCHAR) return 0;
    return __tgi_vfont->widths[a - TGI_VF_FIRSTCHAR];
}

unsigned tgi_gettextwidth(const char *s) {
    unsigned w = 0;
    if (tgi_font == TGI_FONT_BITMAP) return (unsigned)strlen(s) * (tgi_charwidth >> 8);
    if (!__tgi_vfont) return 0;
    while (*s) w += vwidth(ascii((unsigned char)*s++));
    return (unsigned)tgi_imulround((int)w, (int)tgi_textscalew[0]);
}

unsigned tgi_gettextheight(const char *s) {
    (void)s;
    if (tgi_font == TGI_FONT_BITMAP) return tgi_charheight >> 8;
    if (!__tgi_vfont) return 0;
    return (unsigned)tgi_imulround(__tgi_vfont->top, (int)tgi_textscaleh[0]);
}

/* ============================================================================
 * Vector fonts
 * ========================================================================== */

/* An operation's 7-bit signed value */
static int opval(unsigned char b) {
    return (signed char)(unsigned char)(b << 1) >> 1;
}

void tgi_vectorchar(char ch) {
    unsigned char a = (unsigned char)ch, bx, by;
    const unsigned char *ops;
    int x1 = tgi_curx, y1 = tgi_cury, x2, y2, xs, ys;
    if (!__tgi_vfont || a < TGI_VF_FIRSTCHAR || a > TGI_VF_LASTCHAR) return;
    ops = __tgi_vfont->chars[a - TGI_VF_FIRSTCHAR];
    do {
        bx = ops[0];
        by = ops[1];
        ops += 2;
        xs = tgi_imulround(opval(bx), (int)tgi_textscalew[0]);
        ys = tgi_imulround(opval(by), (int)tgi_textscaleh[0]);
        if (tgi_textdir == TGI_TEXT_HORIZONTAL) {
            x2 = tgi_curx + xs;
            y2 = tgi_cury - ys;
        } else {
            y2 = tgi_cury - xs;
            x2 = tgi_curx - ys;
        }
        if (by & 0x80) __tgi_clipline(x1, y1, x2, y2);
        x1 = x2;
        y1 = y2;
    } while (!(bx & 0x80));
}

/* ============================================================================
 * Output
 * ========================================================================== */

/* The font's lower/upper case half: CHARPTR (the character ROM for the
 * VIC-II's $1000/$1800/$9000/$9800), its second 2 KB */
static unsigned long font_addr(void) {
    unsigned long a = (unsigned long)REG(0xD068) | ((unsigned long)REG(0xD069) << 8) |
                      ((unsigned long)REG(0xD06A) << 16);
    unsigned int lo = (unsigned int)a & 0xF000u;
    if (a < 0x10000UL && (lo == 0x1000u || lo == 0x9000u))
        a = 0xFF7E000UL + ((unsigned int)a & 0x0FFFu);
    return (a & ~0x7FFUL) | 0x800u;
}

/* PETSCII c's screen code, $FF for a control character */
static unsigned char scrcode(unsigned char c) {
    if (c < 0x20 || (c >= 0x80 && c < 0xA0)) return 0xFF;
    if (c < 0x40) return c;
    if (c < 0x60) return (unsigned char)(c - 0x40);
    if (c < 0x80) return (unsigned char)(c - 0x20);
    if (c < 0xC0) return (unsigned char)(c - 0x40);
    if (c == 0xFF) return 0x5E;
    return (unsigned char)(c - 0x80);
}

/* Move the cursor along the text by w pixels */
static void advance(unsigned w) {
    if (tgi_textdir == TGI_TEXT_HORIZONTAL)
        tgi_curx += (int)w;
    else
        tgi_cury -= (int)w;
}

/* stdlib_asm/tgi_pixel.s45: an 8x8 glyph wholly on the screen at 1x */
__fastcall__ void __tgi_glyph(int x, int y, unsigned long glyph);

/* A bitmap character at the cursor: wholly on the screen at 1x, stored
 * directly; else each row's runs of set pixels a bar */
static void bitmap_char(unsigned long font, unsigned char code, unsigned char mx, unsigned char my) {
    unsigned long ga = font + ((unsigned int)code << 3);
    const unsigned char __far *g = (const unsigned char __far *)ga;
    unsigned char r, c, e, bits;
    int x = tgi_curx, y = tgi_cury, a, b;
    if (mx == 1 && my == 1 && tgi_textdir == TGI_TEXT_HORIZONTAL &&
        x >= 0 && y >= 0 && x <= (int)tgi_xmax - 7 && y <= (int)tgi_ymax - 7) {
        __tgi_glyph(x, y, ga);
        return;
    }
    for (r = 0; r < 8; r++) {
        bits = g[r];
        c = 0;
        while (bits) {
            if (!(bits & 0x80)) {
                bits <<= 1;
                c++;
                continue;
            }
            e = c;
            while (bits & 0x80) {
                bits <<= 1;
                e++;
            }
            /* pixels c .. e-1 of row r */
            if (tgi_textdir == TGI_TEXT_HORIZONTAL) {
                a = y + (int)r * my;
                tgi_bar(x + (int)c * mx, a, x + (int)e * mx - 1, a + my - 1);
            } else {
                b = x + (int)r * my;
                tgi_bar(b, y - (int)e * mx + 1, b + my - 1, y - (int)c * mx);
            }
            c = e;
        }
    }
}

void tgi_outtext(const char *s) {
    unsigned char c, mx, my;
    unsigned long font;
    if (tgi_font != TGI_FONT_BITMAP) {
        if (!__tgi_vfont) return;
        while (*s) {
            c = ascii((unsigned char)*s++);
            tgi_vectorchar((char)c);
            advance((unsigned)tgi_imulround(vwidth(c), (int)tgi_textscalew[0]));
        }
        return;
    }
    mx = (unsigned char)(tgi_textscalew[1] >> 8);
    my = (unsigned char)(tgi_textscaleh[1] >> 8);
    font = font_addr();
    while (*s) {
        c = scrcode((unsigned char)*s++);
        if (c != 0xFF && mx && my) bitmap_char(font, c, mx, my);
        advance(tgi_charwidth >> 8);
    }
}

void tgi_outtextxy(int x, int y, const char *s) {
    tgi_curx = x;
    tgi_cury = y;
    tgi_outtext(s);
}
