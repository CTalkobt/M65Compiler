/* tgi.c — <tgi.h>'s kernel and its VIC-IV full-colour screen
 *
 * The screen is 40x25 full-colour characters (CHR16, FCLRHI): a character
 * is 64 bytes, a byte a pixel, at its number * 64. A page's 1000 characters
 * are in one block, a column of 25 after another, so pixel (x, y) is at
 *   page + (x >> 3) * 1600 + y * 8 + (x & 7)
 * and a column of a character column (x fixed) is every 8th byte. Pixel
 * $00 shows $D021 and $FF the colour RAM's colour (multicolour mode on: all
 * 8 bits), so with $D021 0 and the colour RAM $FF a pixel's byte is its
 * palette entry.
 *
 * tgi_install takes the memory, far_malloc blocks in bank 4 or 5: a page
 * (64000 bytes, 64-byte aligned; such a block starts low enough in its bank
 * that the page never crosses into the next one), the palette as it was
 * (768 bytes: tgi_setpalette's entries are colours of that one) and a
 * page's screen (2000 bytes, the characters' numbers). One page leaves room
 * for the others; with two, what does not fit comes from malloc.
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <dma.h>
#include <far_ptr.h>
#include <stdlib.h>
#include "tgi_drv.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define PIXP(a) ((unsigned char __far *)(a))
#define COLOUR_RAM 0xFF80000UL
#define TGI_COLOURS 0x3000u     /* the screen's colour RAM (2000 bytes) */
#define PAGE_SIZE 64000u
#define COL_SIZE 1600u          /* a character column: 25 characters */

/* TGI kernel variables */
const void *tgi_drv;
unsigned char tgi_error;
unsigned char tgi_gmode;
signed int tgi_curx, tgi_cury;
unsigned char tgi_color;
unsigned char tgi_font;
unsigned char tgi_textdir;
const tgi_vectorfont *__tgi_vfont;
unsigned tgi_xres, tgi_yres, tgi_xmax, tgi_ymax;
unsigned char tgi_colorcount, tgi_pagecount, tgi_fontwidth, tgi_fontheight;
unsigned tgi_aspectratio;
unsigned char tgi_flags;
unsigned tgi_textscalew[2], tgi_textscaleh[2];
unsigned tgi_charwidth, tgi_charheight;

/* The driver */
unsigned long __tgi_draw;               /* the draw page */
static unsigned char view;              /* the view page */
static far_ptr_t fblk[5];               /* the memory: pages, screens, palette */
static void *hblk[5];                   /* (from malloc) */
static unsigned long page[2];           /* the pages */
static unsigned long scr[2];            /* their screens */
static unsigned long palsave;           /* the palette at tgi_install: R, G, B (768 bytes) */
static unsigned char palset;            /* tgi_setpalette changed the palette */
static unsigned char pal[256];          /* tgi_getpalette */
static unsigned char old[17];           /* the VIC registers tgi_init changes */

/* The registers tgi_init sets, in the order tgi_done puts them back ($D031
 * first: writing it sets the VIC-IV's layout for the mode, which the others
 * then change) */
static const unsigned int vicregs[17] = {
    0xD031, 0xD011, 0xD016, 0xD020, 0xD021, 0xD054, 0xD058, 0xD059, 0xD05E,
    0xD060, 0xD061, 0xD062, 0xD063, 0xD064, 0xD065, 0xD04C, 0xD04E
};

static const unsigned int coloff[40] = {
    0u * COL_SIZE, 1u * COL_SIZE, 2u * COL_SIZE, 3u * COL_SIZE, 4u * COL_SIZE,
    5u * COL_SIZE, 6u * COL_SIZE, 7u * COL_SIZE, 8u * COL_SIZE, 9u * COL_SIZE,
    10u * COL_SIZE, 11u * COL_SIZE, 12u * COL_SIZE, 13u * COL_SIZE, 14u * COL_SIZE,
    15u * COL_SIZE, 16u * COL_SIZE, 17u * COL_SIZE, 18u * COL_SIZE, 19u * COL_SIZE,
    20u * COL_SIZE, 21u * COL_SIZE, 22u * COL_SIZE, 23u * COL_SIZE, 24u * COL_SIZE,
    25u * COL_SIZE, 26u * COL_SIZE, 27u * COL_SIZE, 28u * COL_SIZE, 29u * COL_SIZE,
    30u * COL_SIZE, 31u * COL_SIZE, 32u * COL_SIZE, 33u * COL_SIZE, 34u * COL_SIZE,
    35u * COL_SIZE, 36u * COL_SIZE, 37u * COL_SIZE, 38u * COL_SIZE, 39u * COL_SIZE
};

/* Pixel (x, y)'s offset in a page (x, y on the screen) */
#define OFF(x, y) (coloff[(unsigned)(x) >> 3] + ((unsigned)(y) << 3) + ((unsigned)(x) & 7u))

static const char *const msgs[] = {
    "No error",
    "No driver available",
    "Cannot load driver",
    "Invalid driver",
    "Mode not supported by driver",
    "Invalid function argument",
    "Function not supported",
    "Invalid font file",
    "Out of resources",
    "A driver is already installed",
    "Unknown error"
};

/* ============================================================================
 * The driver's pages
 * ========================================================================== */

static void free_pages(void) {
    unsigned char k;
    for (k = 0; k < 5; k++) {
        if (!far_is_null(fblk[k])) far_free(fblk[k]);
        fblk[k] = 0;
        free(hblk[k]);
        hblk[k] = 0;
    }
}

/* Block k: size bytes in bank 4 or 5, else (heap) from malloc; its address
 * (0: none) */
static unsigned long take(unsigned char k, unsigned int size, unsigned char heap) {
    far_ptr_t m = far_malloc(size, BANK4);
    if (far_is_null(m)) m = far_malloc(size, BANK5);
    if (!far_is_null(m)) {
        fblk[k] = m;
        return far_to_linear(m);
    }
    if (heap && (hblk[k] = malloc(size)) != 0) return (unsigned long)(unsigned int)hblk[k];
    return 0;
}

/* Page k (cleared) */
static unsigned char take_page(unsigned char k) {
    unsigned long a = take(k, PAGE_SIZE + 63u, 0);
    if (!a) return 0;
    page[k] = (a + 63u) & ~63UL;
    dma_fill_far(page[k], PAGE_SIZE, 0);
    return 1;
}

/* Page k's screen */
static unsigned char take_screen(unsigned char k) {
    unsigned int n = (unsigned int)(page[k] >> 6), i;
    unsigned char col, row;
    unsigned char __far *s;
    scr[k] = take(2 + k, 2000, 1);
    if (!scr[k]) return 0;
    s = PIXP(scr[k]);
    for (col = 0; col < 40; col++) {
        i = 2u * col;
        for (row = 0; row < 25; row++) {
            s[i] = (unsigned char)n;
            s[i + 1] = (unsigned char)(n >> 8);
            n++;
            i += 80;
        }
    }
    return 1;
}

static void show_page(unsigned char k) {
    unsigned long a = scr[k];
    REG(0xD060) = (unsigned char)a;
    REG(0xD061) = (unsigned char)(a >> 8);
    REG(0xD062) = (unsigned char)(a >> 16);
    REG(0xD063) = (REG(0xD063) & 0xF0) | ((unsigned char)(a >> 24) & 0x0F);
}

/* ============================================================================
 * Driver lifecycle
 * ========================================================================== */

unsigned char tgi_install(const void *driver) {
    const tgi_driver_t *d = (const tgi_driver_t *)driver;
    unsigned int i;
    unsigned char __far *p;
    if (tgi_drv) return tgi_error = TGI_ERR_INSTALLED;
    if (!d || d->sig[0] != 0x74 || d->sig[1] != 0x67 || d->sig[2] != 0x69 ||
        d->sig[3] != TGI_API_VERSION || d->pagecount < 1 || d->pagecount > 2)
        return tgi_error = TGI_ERR_INV_DRIVER;
    /* the pages, then what fits beside them */
    if (!take_page(0) || (d->pagecount > 1 && !take_page(1)) ||
        (palsave = take(4, 768, 1)) == 0 ||
        !take_screen(0) || (d->pagecount > 1 && !take_screen(1))) {
        free_pages();
        return tgi_error = TGI_ERR_NO_RES;
    }
    p = PIXP(palsave);
    for (i = 0; i < 768; i++) p[i] = REG(0xD100 + i);
    palset = 0;
    for (i = 0; i < 256; i++) pal[i] = (unsigned char)i;
    tgi_drv = driver;
    tgi_xres = d->xres;
    tgi_yres = d->yres;
    tgi_xmax = d->xres - 1;
    tgi_ymax = d->yres - 1;
    tgi_colorcount = d->colorcount;
    tgi_pagecount = d->pagecount;
    tgi_fontwidth = d->fontwidth;
    tgi_fontheight = d->fontheight;
    tgi_aspectratio = d->aspectratio;
    tgi_flags = d->flags;
    /* what a new driver starts with */
    tgi_error = TGI_ERR_OK;
    tgi_gmode = 0;
    tgi_curx = tgi_cury = 0;
    tgi_color = 0;
    tgi_font = TGI_FONT_BITMAP;
    tgi_textdir = TGI_TEXT_HORIZONTAL;
    __tgi_vfont = 0;
    tgi_textscalew[0] = tgi_textscalew[1] = tgi_charwidth = 0;
    tgi_textscaleh[0] = tgi_textscaleh[1] = tgi_charheight = 0;
    __tgi_draw = page[0];
    view = 0;
    return TGI_ERR_OK;
}

void tgi_uninstall(void) {
    tgi_done();
    free_pages();
    tgi_drv = 0;
    tgi_error = TGI_ERR_OK;
}

void tgi_unload(void) {
    if (!tgi_drv) {
        tgi_error = TGI_ERR_NO_DRIVER;
        return;
    }
    tgi_uninstall();
}

void tgi_init(void) {
    unsigned char i;
    if (!tgi_drv) {
        tgi_error = TGI_ERR_NO_DRIVER;
        return;
    }
    tgi_done();
    for (i = 0; i < 17; i++) old[i] = REG(vicregs[i]);
    REG(0xD031) = old[0] & 0x77;        /* 320x200: H640, V400 off */
    REG(0xD011) = old[1] & 0x9F;        /* text: no bitmap, no ECM */
    REG(0xD016) = old[2] | 0x10;        /* multicolour: 8-bit colour RAM colours */
    REG(0xD020) = 0;
    REG(0xD021) = 0;
    REG(0xD054) = old[5] | 0x05;        /* CHR16, FCLRHI */
    REG(0xD058) = 80;                   /* 2 bytes a character */
    REG(0xD059) = 0;
    REG(0xD05E) = 40;
    REG(0xD064) = (unsigned char)TGI_COLOURS;
    REG(0xD065) = (unsigned char)(TGI_COLOURS >> 8);
    dma_fill_far(COLOUR_RAM + TGI_COLOURS, 2000, 0);
    dma_fill_far_step(COLOUR_RAM + TGI_COLOURS + 1, 2, 1000, 0xFF);
    tgi_gmode = 1;
    tgi_setviewpage(0);
    tgi_setdrawpage(0);
    tgi_setpalette(tgi_getdefpalette());
    tgi_error = TGI_ERR_OK;
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
}

void tgi_done(void) {
    unsigned char i;
    unsigned int k;
    unsigned char __far *p;
    if (!tgi_gmode) return;
    REG(0xD054) = old[5];
    for (i = 0; i < 17; i++) REG(vicregs[i]) = old[i];
    if (palset) {
        p = PIXP(palsave);
        for (k = 0; k < 768; k++) REG(0xD100 + k) = p[k];
        palset = 0;
    }
    tgi_gmode = 0;
}

/* ============================================================================
 * Errors, control
 * ========================================================================== */

unsigned char tgi_geterror(void) {
    unsigned char e = tgi_error;
    tgi_error = TGI_ERR_OK;
    return e;
}

const char *tgi_geterrormsg(unsigned char code) {
    if (code > TGI_ERR_INSTALLED) code = TGI_ERR_INSTALLED + 1;
    return msgs[code];
}

unsigned tgi_ioctl(unsigned char code, void *data) {
    if (tgi_drv) {
        if (code == TGI_IOCTL_DRAWPAGE_ADDR) {
            *(unsigned long *)data = __tgi_draw;
            return TGI_ERR_OK;
        }
        if (code == TGI_IOCTL_VIEWPAGE_ADDR) {
            *(unsigned long *)data = page[view];
            return TGI_ERR_OK;
        }
    }
    tgi_error = TGI_ERR_INV_FUNC;
    return TGI_ERR_INV_FUNC;
}

/* ============================================================================
 * Pages, colours, the palette
 * ========================================================================== */

unsigned tgi_getpagecount(void) {
    return tgi_pagecount;
}

void tgi_setviewpage(unsigned char k) {
    if (k >= tgi_pagecount) {
        tgi_error = TGI_ERR_INV_ARG;
        return;
    }
    view = k;
    if (tgi_gmode) show_page(k);
}

void tgi_setdrawpage(unsigned char k) {
    if (k >= tgi_pagecount) {
        tgi_error = TGI_ERR_INV_ARG;
        return;
    }
    __tgi_draw = page[k];
}

unsigned char tgi_getcolorcount(void) {
    return tgi_colorcount;
}

unsigned char tgi_getmaxcolor(void) {
    return (unsigned char)(tgi_colorcount - 1);
}

void tgi_setcolor(unsigned char c) {
    if (tgi_colorcount && c >= tgi_colorcount) {
        tgi_error = TGI_ERR_INV_ARG;
        return;
    }
    tgi_color = c;
}

unsigned char tgi_getcolor(void) {
    return tgi_color;
}

void tgi_setpalette(const unsigned char *p) {
    unsigned int i;
    unsigned char c, k;
    unsigned char __far *s = PIXP(palsave);
    if (!tgi_drv) return;
    for (i = 0; i < 256; i++) {
        c = p[i];
        pal[i] = c;
        if (c != (unsigned char)i) palset = 1;
    }
    /* (all entries: back to the default one too) */
    for (i = 0; i < 256; i++) {
        c = pal[i];
        for (k = 0; k < 3; k++) REG(0xD100 + ((unsigned int)k << 8) + i) = s[((unsigned int)k << 8) + c];
    }
}

const unsigned char *tgi_getpalette(void) {
    return pal;
}

/* The default palette: each entry its own colour */
#define P16(n) n, n + 1, n + 2, n + 3, n + 4, n + 5, n + 6, n + 7, \
               n + 8, n + 9, n + 10, n + 11, n + 12, n + 13, n + 14, n + 15
static const unsigned char defpal[256] = {
    P16(0), P16(16), P16(32), P16(48), P16(64), P16(80), P16(96), P16(112),
    P16(128), P16(144), P16(160), P16(176), P16(192), P16(208), P16(224), P16(240)
};

const unsigned char *tgi_getdefpalette(void) {
    return defpal;
}

unsigned tgi_getxres(void) {
    return tgi_xres;
}

unsigned tgi_getmaxx(void) {
    return tgi_xmax;
}

unsigned tgi_getyres(void) {
    return tgi_yres;
}

unsigned tgi_getmaxy(void) {
    return tgi_ymax;
}

unsigned tgi_getaspectratio(void) {
    return tgi_aspectratio;
}

void tgi_setaspectratio(unsigned r) {
    tgi_aspectratio = r;
}

/* ============================================================================
 * Drawing
 * ========================================================================== */

void tgi_clear(void) {
    if (tgi_drv) dma_fill_far(__tgi_draw, PAGE_SIZE, 0);
}

void tgi_gotoxy(int x, int y) {
    tgi_curx = x;
    tgi_cury = y;
}

/* tgi_setpixel, tgi_getpixel and the line on the screen are assembly
 * (stdlib_asm/tgi_pixel.s45) */
__fastcall__ void __tgi_line_on_screen(int x1, int y1, int x2, int y2);

/* Cohen-Sutherland outcode */
static unsigned char outcode(signed long x, signed long y) {
    unsigned char c = 0;
    if (x < 0) c = 1;
    else if (x > (signed long)tgi_xmax) c = 2;
    if (y < 0) c |= 4;
    else if (y > (signed long)tgi_ymax) c |= 8;
    return c;
}

/* A line with an end off the screen, clipped (tgi_pixel.s45's
 * __tgi_clipline draws one wholly on the screen itself; tgi_line and
 * tgi_lineto are there too) */
__fastcall__ void __tgi_clipline_c(int x1, int y1, int x2, int y2) {
    signed long ax, ay, bx, by, x, y, xm, ym;
    unsigned char c1, c2, c;
    ax = x1;
    ay = y1;
    bx = x2;
    by = y2;
    xm = (signed long)tgi_xmax;
    ym = (signed long)tgi_ymax;
    c1 = outcode(ax, ay);
    c2 = outcode(bx, by);
    while (c1 | c2) {
        if (c1 & c2) return;
        c = c1 ? c1 : c2;
        if (c & 8) {
            y = ym;
            x = ax + (bx - ax) * (ym - ay) / (by - ay);
        } else if (c & 4) {
            y = 0;
            x = ax + (bx - ax) * (0 - ay) / (by - ay);
        } else if (c & 2) {
            x = xm;
            y = ay + (by - ay) * (xm - ax) / (bx - ax);
        } else {
            x = 0;
            y = ay + (by - ay) * (0 - ax) / (bx - ax);
        }
        if (c == c1) {
            ax = x;
            ay = y;
            c1 = outcode(ax, ay);
        } else {
            bx = x;
            by = y;
            c2 = outcode(bx, by);
        }
    }
    __tgi_line_on_screen((int)ax, (int)ay, (int)bx, (int)by);
}

void tgi_bar(int x1, int y1, int x2, int y2) {
    int t;
    unsigned int h;
    unsigned char c = tgi_color;
    if (!tgi_drv) return;
    if (x1 > x2) { t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { t = y1; y1 = y2; y2 = t; }
    if (x2 < 0 || y2 < 0 || x1 > (int)tgi_xmax || y1 > (int)tgi_ymax) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > (int)tgi_xmax) x2 = (int)tgi_xmax;
    if (y2 > (int)tgi_ymax) y2 = (int)tgi_ymax;
    h = (unsigned int)(y2 - y1 + 1);
    while (x1 <= x2) {
        if (!(x1 & 7) && x1 + 7 <= x2) {
            /* a whole character column: its rows are one run */
            dma_fill_far(__tgi_draw + OFF(x1, y1), h << 3, c);
            x1 += 8;
        } else {
            dma_fill_far_step(__tgi_draw + OFF(x1, y1), 8, h, c);
            x1++;
        }
    }
}
