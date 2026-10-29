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
 * tgi_install, tgi_uninstall, tgi_init, tgi_done, tgi_setviewpage and
 * tgi_setdrawpage, with the memory they take, are stdlib_asm/tgi_setup.s45;
 * the pixels, lines and ellipses tgi_pixel.s45, the arcs tgi_arc.s45.
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <dma.h>
#include "tgi_drv.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define PIXP(a) ((unsigned char __far *)(a))
#define PAGE_SIZE 64000u
#define COL_SIZE 1600u          /* a character column: 25 characters */

/* tgi_setup.s45's */
extern unsigned long __tgi_draw;        /* the draw page */
extern unsigned char __tgi_view;        /* the view page */
extern unsigned long __tgi_page[2];     /* the pages */
extern unsigned long __tgi_palsave;     /* the palette at tgi_install: R, G, B (768 bytes) */
extern unsigned char __tgi_palset;      /* tgi_setpalette changed the palette */
extern unsigned char __tgi_mode;        /* tgi_vars.s45's: $80, the 640x200 driver */

/* The 640x200 driver's bar (tgi_ncm.s45, linked with it) */
#pragma weak __tgi_n_bar
__fastcall__ void __tgi_n_bar(int x1, int y1, int x2, int y2) { (void)x1; (void)y1; (void)x2; (void)y2; }
static unsigned char pal[256];          /* tgi_getpalette (after tgi_setpalette) */
static unsigned char defpal[256];       /* tgi_getdefpalette's */

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
 * Driver lifecycle
 * ========================================================================== */

void tgi_unload(void) {
    if (!tgi_drv) {
        tgi_error = TGI_ERR_NO_DRIVER;
        return;
    }
    tgi_uninstall();
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
            *(unsigned long *)data = __tgi_page[__tgi_view];
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
    unsigned int i, n = tgi_colorcount ? tgi_colorcount : 256;   /* (its entries) */
    unsigned char c, k;
    unsigned char __far *s = PIXP(__tgi_palsave);
    if (!tgi_drv) return;
    for (i = 0; i < 256; i++) {
        c = i < n ? p[i] : (unsigned char)i;
        pal[i] = c;
        if (c != (unsigned char)i) __tgi_palset = 1;
    }
    /* (all entries: back to the default one too) */
    for (i = 0; i < 256; i++) {
        c = pal[i];
        for (k = 0; k < 3; k++) REG(0xD100 + ((unsigned int)k << 8) + i) = s[((unsigned int)k << 8) + c];
    }
}

const unsigned char *tgi_getpalette(void) {
    return __tgi_palset ? pal : tgi_getdefpalette();
}

/* The default palette: each entry its own colour (made here: no 256-byte
 * table in the program) */
const unsigned char *tgi_getdefpalette(void) {
    unsigned int i;
    for (i = 0; i < 256; i++) defpal[i] = (unsigned char)i;
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
    if (__tgi_mode) {
        __tgi_n_bar(x1, y1, x2, y2);
        return;
    }
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
