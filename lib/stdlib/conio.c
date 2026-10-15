/* conio.c — Console I/O on the MEGA65 text screen
 *
 * Works on the screen the VIC-IV shows: its screen RAM ($D060-$D063), the
 * row stride ($D058/$D059), characters per row ($D05E), rows ($D07B + 1) and the
 * colour RAM offset ($D064/$D065), so 40 and 80 columns, 25 and 50 rows and
 * a moved screen all work. Characters are PETSCII (what cc45 makes of C
 * text) converted to screen codes; colour RAM ($FF80000) is written by DMA.
 * The text colour is a colour RAM byte's low nibble; there is no per-character
 * background in text mode, so textbackground sets the screen's ($D021).
 */

#pragma cc45 signed_default

#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dma.h>

#define VIC(r) (*(volatile unsigned char *)(0xD000 + (r)))
#define COLOUR_RAM 0xFF80000UL

static int cursor_x, cursor_y;
static unsigned char cur_color = WHITE;
static unsigned char cursor_shown;
static unsigned char reverse_on;   /* revers(): bit 7 of the screen code */

/* ============================================================================
 * SCREEN GEOMETRY (read from the VIC-IV each time: a program may change it)
 * ============================================================================ */

int screenwidth(void) {
    unsigned char w = VIC(0x5E);
    return w ? w : 40;
}

/* $D07B holds the number of rows minus 1 (24, or 49 with V400) */
int screenheight(void) {
    return VIC(0x7B) + 1;
}

static unsigned int line_step(void) {
    return VIC(0x58) | ((unsigned int)VIC(0x59) << 8);
}

/* The screen RAM address of (x, y) */
static unsigned long scr_addr(int x, int y) {
    unsigned long base = VIC(0x60) | ((unsigned int)VIC(0x61) << 8) | ((unsigned long)VIC(0x62) << 16) |
                         ((unsigned long)(VIC(0x63) & 0x0F) << 24);
    return base + (unsigned long)y * line_step() + (unsigned int)x;
}

/* The colour RAM address of (x, y) */
static unsigned long col_addr(int x, int y) {
    unsigned int off = VIC(0x64) | ((unsigned int)VIC(0x65) << 8);
    return COLOUR_RAM + off + (unsigned long)y * line_step() + (unsigned int)x;
}

static void scr_put(unsigned long a, unsigned char code) {
    if (a < 0x10000UL) *(volatile unsigned char *)(unsigned int)a = code;
    else dma_fill_far(a, 1, code);
}

static unsigned char scr_get(unsigned long a) {
    unsigned char c;
    if (a < 0x10000UL) return *(volatile unsigned char *)(unsigned int)a;
    dma_copy_far((unsigned long)(unsigned int)&c, a, 1);
    return c;
}

static unsigned char col_get(unsigned long a) {
    unsigned char c;
    dma_copy_far((unsigned long)(unsigned int)&c, a, 1);
    return c;
}

/* PETSCII to screen code (control characters give 0xFF: not printed) */
static unsigned char petscii_to_screen(unsigned char c) {
    if (c < 0x20) return 0xFF;
    if (c < 0x40) return c;
    if (c < 0x60) return c - 0x40;
    if (c < 0x80) return c - 0x20;
    if (c < 0xA0) return 0xFF;
    if (c < 0xC0) return c - 0x40;
    if (c < 0xFF) return c - 0x80;
    return 0x5E;
}

/* The rows below the top one up by a row; the bottom row cleared */
static void scroll_up(void) {
    int w = screenwidth(), h = screenheight();
    unsigned int step = line_step();
    unsigned int n = (unsigned int)(h - 1) * step;
    dma_copy_far(scr_addr(0, 0), scr_addr(0, 1), n);
    dma_copy_far(col_addr(0, 0), col_addr(0, 1), n);
    dma_fill_far(scr_addr(0, h - 1), (unsigned int)w, 0x20);
    dma_fill_far(col_addr(0, h - 1), (unsigned int)w, cur_color);
}

static void newline(void) {
    cursor_x = 0;
    if (++cursor_y >= screenheight()) {
        cursor_y = screenheight() - 1;
        scroll_up();
    }
}

static void clamp_cursor(void) {
    int w = screenwidth(), h = screenheight();
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= w) cursor_x = w - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= h) cursor_y = h - 1;
}

/* ============================================================================
 * SCREEN CONTROL
 * ============================================================================ */

void clrscr(void) {
    int y, w = screenwidth(), h = screenheight();
    unsigned int step = line_step();
    if (step == (unsigned int)w) {
        dma_fill_far(scr_addr(0, 0), (unsigned int)(w * h), 0x20);
        dma_fill_far(col_addr(0, 0), (unsigned int)(w * h), cur_color);
    } else {
        for (y = 0; y < h; y++) {
            dma_fill_far(scr_addr(0, y), (unsigned int)w, 0x20);
            dma_fill_far(col_addr(0, y), (unsigned int)w, cur_color);
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

void gotoxy(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    clamp_cursor();
}

int wherex(void) {
    return cursor_x;
}

int wherey(void) {
    return cursor_y;
}

/* ============================================================================
 * TEXT ATTRIBUTES
 * ============================================================================ */

void textcolor(int color) {
    cur_color = (unsigned char)(color & 0x0F);
}

/* Text mode has no per-character background: the screen's ($D021) */
void textbackground(int color) {
    VIC(0x21) = (unsigned char)(color & 0x0F);
}

/* Reversed characters from now on (1) or not (0); returns the old setting */
unsigned char revers(unsigned char onoff) {
    unsigned char old = reverse_on;
    reverse_on = onoff ? 0x80 : 0;
    return old ? 1 : 0;
}

void textattr(int attr) {
    textcolor(attr & 0x0F);
    textbackground((attr >> 4) & 0x0F);
}

/* ============================================================================
 * CHARACTER OUTPUT
 * ============================================================================ */

/* ch at (x, y) in the text colour; 0 for a control character */
static int put_at(int x, int y, int ch) {
    unsigned char code = petscii_to_screen((unsigned char)ch);
    if (code == 0xFF) return 0;
    scr_put(scr_addr(x, y), code | reverse_on);
    dma_fill_far(col_addr(x, y), 1, cur_color);
    return 1;
}

/* At the cursor, which then advances (wrapping, scrolling at the bottom);
 * '\n' ($0D, or $0A from ASCII text) starts the next line */
int putch(int ch) {
    unsigned char c = (unsigned char)ch;
    if (c == 0x0D || c == 0x0A) {
        newline();
        return ch;
    }
    if (put_at(cursor_x, cursor_y, ch) && ++cursor_x >= screenwidth()) newline();
    return ch;
}

void cputch(int x, int y, int ch) {
    put_at(x, y, ch);
}

int cputs(const char *str) {
    if (!str) {
        return -1;
    }
    while (*str) putch((unsigned char)*str++);
    return 0;
}

int cputsxy(int x, int y, const char *str) {
    if (!str) {
        return -1;
    }
    gotoxy(x, y);
    return cputs(str);
}

/* ============================================================================
 * FORMATTED OUTPUT
 * ============================================================================ */

int cprintf(const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, (char *)format, ap);
    va_end(ap);

    cputs(buffer);

    return count;
}

int cprintfxy(int x, int y, const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, (char *)format, ap);
    va_end(ap);

    cputsxy(x, y, buffer);

    return count;
}

/* ============================================================================
 * KEYBOARD INPUT
 * ============================================================================ */

/* The next key (PETSCII), waiting for one; no echo. With cursoron the
 * cursor cell shows reversed while it waits. */
int getch(void) {
    int c;
    unsigned long a = scr_addr(cursor_x, cursor_y);
    unsigned char under = 0;
    if (cursor_shown) {
        under = scr_get(a);
        scr_put(a, under ^ 0x80);
    }
    c = getchar();
    if (cursor_shown) scr_put(a, under);
    return c;
}

/* Whether a key is waiting: in the KERNAL keyboard buffer (count at $00D0)
 * or in the keyboard's hardware queue ($D610, which the C65 KERNAL's GETIN
 * reads directly when its buffer is empty: typed keys wait there) */
int kbhit(void) {
    return *(volatile unsigned char *)0x00D0 != 0 || *(volatile unsigned char *)0xD610 != 0;
}

/* ============================================================================
 * CURSOR VISIBILITY (shown by getch while it waits)
 * ============================================================================ */

void cursoron(void) {
    cursor_shown = 1;
}

void cursoroff(void) {
    cursor_shown = 0;
}

/* ============================================================================
 * SCREEN BUFFER FUNCTIONS
 * ============================================================================ */

/* The screen code at (x, y) */
int getch_xy(int x, int y) {
    return scr_get(scr_addr(x, y));
}

/* The colour RAM byte at (x, y) */
int getattr_xy(int x, int y) {
    return col_get(col_addr(x, y));
}
