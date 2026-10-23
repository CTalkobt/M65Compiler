/* conio_fcm.c — <conio.h>'s full-colour text mode: a background colour for
 * each character
 *
 * The VIC-IV's text mode has one background colour, the screen's ($D021).
 * fcm_enable makes the screen two bytes a cell (CHR16) with full-colour
 * characters for the character numbers from 256 (FCLRHI): such a character
 * is 64 bytes, a byte a pixel, at its number * 64; pixel $00 shows the
 * screen's background, $FF the cell's colour (colour RAM), others that
 * palette entry. A cell on the screen's background shows the font's
 * character as before; a cell on another background a glyph made from the
 * font's 8 bytes, its clear pixels in that colour. The glyphs are kept in a
 * cache (fcm_set_cache_size, 256 by default) in bank 4 or 5 (far_malloc),
 * after the screen (room for 80 columns); when it is full, the glyphs no cell
 * shows are dropped. The colours are in colour RAM from $1000, above the
 * ROM's screen. fcm_disable puts the screen back as it was, with what is on
 * it.
 */
#include <conio.h>
#include <dma.h>
#include <far_ptr.h>

__fastcall__ void __conio_put16(int x, int y, unsigned int n, unsigned char color);
__fastcall__ unsigned int __conio_get16(int x, int y);
__fastcall__ unsigned char __conio_code(unsigned char c);
extern unsigned char *__conio_chtab;

#define REG(a) (*(volatile unsigned char *)(a))
#define COLOUR_RAM 0xFF80000UL
#define FCM_COLOURS 0x1000u     /* the cells' colours in colour RAM (80x50: 8000 bytes) */
#define FCM_BLACK 0xFE          /* palette entry of a black background (pixel $00 is the screen's) */
#define FCM_EMPTY 0xFF          /* fcm_bg of a free cache entry */

static unsigned char fcm_on;
static unsigned int fcm_slots = 256;    /* cache entries */
static far_ptr_t fcm_mem;               /* the screen, then the glyphs */
static unsigned long fcm_glyphs;        /* glyph 0's address */
static unsigned int fcm_first;          /* its character number */
static unsigned char fcm_w;             /* the width fcm_enable found */
static unsigned char fcm_old[9];        /* $D058/$D059, $D060-$D063, $D064/$D065, $D054 */
static unsigned long fcm_font;          /* the font the glyphs are made from */
/* A glyph's character and background by the low byte of its number (glyph
 * i's number is fcm_first + i: at most 256 glyphs, so one byte tells them
 * apart; getch_xy reads fcm_ch through __conio_chtab) */
static unsigned char fcm_ch[256];
static unsigned char fcm_bg[256];
static unsigned char fcm_row[80], fcm_row2[80], fcm_big[160], fcm_pix[64];

/* The linear address of a bank 0 buffer (for DMA) */
#define LIN(p) ((unsigned long)(unsigned int)(p))

int fcm_set_cache_size(int glyphs) {
    if (fcm_on || glyphs < 1 || glyphs > 256) return 0;
    fcm_slots = (unsigned int)glyphs;
    return 1;
}

int fcm_is_enabled(void) {
    return fcm_on;
}

/* The font's address: CHARPTR, the character ROM for the VIC-II's
 * $1000/$1800/$9000/$9800 */
static unsigned long font_addr(void) {
    unsigned long a = (unsigned long)REG(0xD068) | ((unsigned long)REG(0xD069) << 8) |
                      ((unsigned long)REG(0xD06A) << 16);
    unsigned int lo = (unsigned int)a & 0xF000u;
    if (a < 0x10000UL && (lo == 0x1000u || lo == 0x9000u))
        return 0xFF7E000UL + ((unsigned int)a & 0x0FFFu);
    return a;
}

/* The glyph of screen code c on background bg: a cached one, a new one, or
 * 0 when the cache is full of glyphs on the screen */
static unsigned int fcm_free_unused(void);

static unsigned int glyph(unsigned char c, unsigned char bg) {
    unsigned int i, s, free = 0xFFFFu;
    unsigned char k, b, r, pix;
    for (s = 0; s < fcm_slots; s++) {
        k = (unsigned char)(fcm_first + s);
        if (fcm_bg[k] == bg) {
            if (fcm_ch[k] == c) return fcm_first + s;
        } else if (fcm_bg[k] == FCM_EMPTY && free == 0xFFFFu) {
            free = s;
        }
    }
    if (free == 0xFFFFu) {
        free = fcm_free_unused();
        if (free == 0xFFFFu) return 0;
    }
    /* the font's 8 bytes, a byte a pixel: set $FF (the colour), clear bg */
    dma_copy_far(LIN(fcm_row), fcm_font + (unsigned int)c * 8u, 8);
    pix = bg ? bg : FCM_BLACK;
    i = 0;
    for (r = 0; r < 8; r++) {
        b = fcm_row[r];
        for (k = 0; k < 8; k++) {
            fcm_pix[i++] = (b & 0x80) ? 0xFF : pix;
            b <<= 1;
        }
    }
    dma_copy_far(fcm_glyphs + ((unsigned long)free << 6), LIN(fcm_pix), 64);
    k = (unsigned char)(fcm_first + free);
    fcm_ch[k] = c;
    fcm_bg[k] = bg;
    return fcm_first + free;
}

/* Drop the glyphs no cell shows; the first free entry, or $FFFF */
static unsigned int fcm_free_unused(void) {
    unsigned char used[32];
    unsigned char w = (unsigned char)screenwidth(), h = (unsigned char)screenheight(), x, y;
    unsigned int n, s, free = 0xFFFFu;
    unsigned char k;
    for (k = 0; k < 32; k++) used[k] = 0;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            n = __conio_get16(x, y);
            if (n >= fcm_first && n - fcm_first < fcm_slots) {
                k = (unsigned char)n;
                used[k >> 3] |= (unsigned char)(1 << (k & 7));
            }
        }
    for (s = 0; s < fcm_slots; s++) {
        k = (unsigned char)(fcm_first + s);
        if (!(used[k >> 3] & (1 << (k & 7)))) {
            fcm_bg[k] = FCM_EMPTY;
            if (free == 0xFFFFu) free = s;
        }
    }
    return free;
}

/* Screen code c in colour fg on background bg at (x, y) */
static void put_code(int x, int y, unsigned char c, int fg, int bg) {
    unsigned int n = c;
    unsigned char b = (unsigned char)bg & 0x0F;
    if (fcm_on && b != REG(0xD021)) {
        n = glyph(c, b);
        if (!n) n = c;          /* the cache is full: on the screen's background */
    }
    __conio_put16(x, y, n, (unsigned char)fg & 0x0F);
}

/* The screen pointer ($D060-$D063, 28 bits) */
static unsigned long scr_reg(void) {
    return (unsigned long)REG(0xD060) | ((unsigned long)REG(0xD061) << 8) |
           ((unsigned long)REG(0xD062) << 16) | ((unsigned long)(REG(0xD063) & 0x0F) << 24);
}

/* The colour RAM pointer ($D064/$D065) as a linear address */
static unsigned long col_reg(void) {
    return COLOUR_RAM + ((unsigned int)REG(0xD064) | ((unsigned int)REG(0xD065) << 8));
}

/* The w x h cells of the one-byte screen (scr, col, a row every stride
 * bytes) to the two-byte one at fcm_mem / FCM_COLOURS (a row every 2 * w),
 * or back (wide 0) */
static void convert(unsigned long scr, unsigned long col, unsigned int stride,
                    unsigned char w, unsigned char h, unsigned char wide) {
    unsigned long scr2 = far_to_linear(fcm_mem), col2 = COLOUR_RAM + FCM_COLOURS;
    unsigned char y, i, b;
    for (y = 0; y < h; y++) {
        if (wide) {
            dma_copy_far(LIN(fcm_row), scr, w);
            dma_copy_far(LIN(fcm_row2), col, w);
            for (i = 0; i < w; i++) {
                fcm_big[2 * i] = fcm_row[i];
                fcm_big[2 * i + 1] = 0;
            }
            dma_copy_far(scr2, LIN(fcm_big), 2u * w);
            for (i = 0; i < w; i++) {
                fcm_big[2 * i] = 0;
                fcm_big[2 * i + 1] = fcm_row2[i];
            }
            dma_copy_far(col2, LIN(fcm_big), 2u * w);
        } else {
            dma_copy_far(LIN(fcm_big), scr2, 2u * w);
            for (i = 0; i < w; i++) {
                b = fcm_big[2 * i];
                fcm_row[i] = fcm_big[2 * i + 1] ? fcm_ch[b] : b;    /* a glyph: its character */
            }
            dma_copy_far(LIN(fcm_big), col2, 2u * w);
            for (i = 0; i < w; i++) fcm_row2[i] = fcm_big[2 * i + 1];
            dma_copy_far(scr, LIN(fcm_row), w);
            dma_copy_far(col, LIN(fcm_row2), w);
        }
        scr += stride;
        col += stride;
        scr2 += 2u * w;
        col2 += 2u * w;
    }
}

int fcm_enable(void) {
    unsigned char w, h, i;
    unsigned int size;
    unsigned long scr;
    if (fcm_on) return 1;
    w = (unsigned char)screenwidth();
    h = (unsigned char)screenheight();
    /* the screen (2 bytes a cell, 80 columns), the glyphs (64-byte aligned) */
    size = 160u * h + fcm_slots * 64u + 63u;
    fcm_mem = far_malloc(size, BANK4);
    if (far_is_null(fcm_mem)) fcm_mem = far_malloc(size, BANK5);
    if (far_is_null(fcm_mem)) return 0;
    scr = far_to_linear(fcm_mem);
    fcm_glyphs = (scr + 160u * h + 63u) & ~63UL;
    fcm_first = (unsigned int)(fcm_glyphs >> 6);
    for (i = 0; i < 8; i++) fcm_old[i] = REG(0xD058 + (i < 2 ? i : i + 6));
    fcm_old[8] = REG(0xD054);
    fcm_w = w;
    fcm_font = font_addr();
    for (size = 0; size < 256; size++) fcm_bg[size] = FCM_EMPTY;
    convert(scr_reg(), col_reg(), (unsigned int)fcm_old[0] | ((unsigned int)fcm_old[1] << 8), w, h, 1);
    REG(0xD1FE) = 0;            /* the black background's palette entry */
    REG(0xD2FE) = 0;
    REG(0xD3FE) = 0;
    REG(0xD060) = (unsigned char)scr;
    REG(0xD061) = (unsigned char)(scr >> 8);
    REG(0xD062) = (unsigned char)(scr >> 16);
    REG(0xD063) = (unsigned char)((fcm_old[5] & 0xF0) | ((unsigned char)(scr >> 24) & 0x0F));
    REG(0xD064) = (unsigned char)FCM_COLOURS;
    REG(0xD065) = (unsigned char)(FCM_COLOURS >> 8);
    REG(0xD058) = (unsigned char)(2u * w);
    REG(0xD059) = 0;
    REG(0xD054) = fcm_old[8] | 0x05;    /* CHR16, FCLRHI */
    __conio_chtab = fcm_ch;
    fcm_on = 1;
    return 1;
}

void fcm_disable(void) {
    unsigned char w, h, i;
    if (!fcm_on) return;
    w = (unsigned char)screenwidth();
    h = (unsigned char)screenheight();
    REG(0xD054) = fcm_old[8];
    if (w == fcm_w) {
        for (i = 0; i < 8; i++) REG(0xD058 + (i < 2 ? i : i + 6)) = fcm_old[i];
    } else {
        REG(0xD031) = REG(0xD031);   /* (conio_set_width since: the VIC's own layout for it) */
    }
    convert(scr_reg(), col_reg(), (unsigned int)REG(0xD058) | ((unsigned int)REG(0xD059) << 8), w, h, 0);
    __conio_chtab = 0;
    far_free(fcm_mem);
    fcm_on = 0;
}

void fcm_putch(int x, int y, int ch, int fg, int bg) {
    unsigned char c = __conio_code((unsigned char)ch);
    if (c == 0xFF || (unsigned int)x >= (unsigned int)screenwidth() ||
        (unsigned int)y >= (unsigned int)screenheight())
        return;
    put_code(x, y, c, fg, bg);
}

int fcm_cputs(int x, int y, const char *str, int fg, int bg) {
    int w = screenwidth(), h = screenheight();
    unsigned char c;
    if (!str) return -1;
    while (*str && y < h) {
        if (*str == '\n' || *str == '\r') {
            x = w;
        } else {
            c = __conio_code((unsigned char)*str);
            if (c != 0xFF && x >= 0 && y >= 0) put_code(x, y, c, fg, bg);
            x++;
        }
        if (x >= w) {
            x = 0;
            y++;
        }
        str++;
    }
    return 0;
}

void fcm_fill_rect(int x1, int y1, int x2, int y2, int ch, int fg, int bg) {
    int w = screenwidth(), h = screenheight(), x, y;
    unsigned char c = __conio_code((unsigned char)ch);
    if (c == 0xFF) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= w) x2 = w - 1;
    if (y2 >= h) y2 = h - 1;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            put_code(x, y, c, fg, bg);
}

void fcm_set_line_color(int y, int fg, int bg) {
    int w = screenwidth(), x;
    if (y < 0 || y >= screenheight()) return;
    for (x = 0; x < w; x++)
        put_code(x, y, (unsigned char)getch_xy(x, y), fg, bg);
}

int fcm_getattr_xy(int x, int y) {
    unsigned char fg = (unsigned char)getattr_xy(x, y) & 0x0F, bg = REG(0xD021) & 0x0F;
    unsigned int n;
    if (fcm_on) {
        n = __conio_get16(x, y);
        if (n >= 256) bg = fcm_bg[(unsigned char)n];
    }
    return (bg << 4) | fg;
}
