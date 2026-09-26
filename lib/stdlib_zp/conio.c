/* conio.c — Console I/O Implementation for MEGA65
 *
 * Provides conio-compatible functions for text mode on MEGA65.
 * Uses VIC-IV screen RAM and hardware registers.
 */

#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mega65.h>

/* ============================================================================
 * SCREEN CONFIGURATION
 * ============================================================================ */

/* MEGA65 screen RAM starts at $0400 (default) */
#define SCREEN_RAM      ((unsigned char *)0x0400)
#define COLOR_RAM       ((unsigned char *)0xD800)

/* Current cursor position and attributes */
static int cursor_x = 0;
static int cursor_y = 0;
static int current_color = WHITE;
static int current_bg = BLACK;
static int screen_width = CONIO_COLS;
static int screen_height = CONIO_ROWS;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * _get_screen_offset - Convert (x, y) to screen RAM offset
 */
static int _get_screen_offset(int x, int y) {
    return y * screen_width + x;
}

/**
 * _clamp_cursor - Ensure cursor is within bounds
 */
static void _clamp_cursor(void) {
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x >= screen_width) cursor_x = screen_width - 1;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y >= screen_height) cursor_y = screen_height - 1;
}

/**
 * _advance_cursor - Move cursor to next position with wrapping
 */
static void _advance_cursor(void) {
    cursor_x++;
    if (cursor_x >= screen_width) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= screen_height) {
            cursor_y = screen_height - 1;
            /* Scroll screen (TODO: implement scrolling) */
        }
    }
}

/* ============================================================================
 * SCREEN CONTROL
 * ============================================================================ */

void clrscr(void) {
    int i;
    int size = screen_width * screen_height;

    /* Clear screen RAM */
    for (i = 0; i < size; i++) {
        SCREEN_RAM[i] = ' ';
    }

    /* Clear color RAM */
    for (i = 0; i < size; i++) {
        COLOR_RAM[i] = (current_bg << 4) | current_color;
    }

    /* Home cursor */
    cursor_x = 0;
    cursor_y = 0;
}

void gotoxy(int x, int y) {
    cursor_x = x;
    cursor_y = y;
    _clamp_cursor();
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
    current_color = color & 0x0F;
}

void textbackground(int color) {
    current_bg = color & 0x0F;
}

void textattr(int attr) {
    current_color = attr & 0x0F;
    current_bg = (attr >> 4) & 0x0F;
}

/* ============================================================================
 * CHARACTER OUTPUT
 * ============================================================================ */

int putch(int ch) {
    int offset = _get_screen_offset(cursor_x, cursor_y);

    SCREEN_RAM[offset] = (unsigned char)ch;
    COLOR_RAM[offset] = (current_bg << 4) | current_color;

    _advance_cursor();

    return ch;
}

void cputch(int x, int y, int ch) {
    int offset = _get_screen_offset(x, y);

    SCREEN_RAM[offset] = (unsigned char)ch;
    COLOR_RAM[offset] = (current_bg << 4) | current_color;
}

int cputs(const char *str) {
    int count = 0;

    if (!str) {
        return -1;
    }

    while (*str) {
        putch(*str);
        str++;
        count++;
    }

    return 0;
}

int cputsxy(int x, int y, const char *str) {
    int count = 0;

    if (!str) {
        return -1;
    }

    gotoxy(x, y);

    while (*str) {
        putch(*str);
        str++;
        count++;
    }

    return 0;
}

/* ============================================================================
 * FORMATTED OUTPUT
 * ============================================================================ */

int cprintf(const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, format, ap);
    va_end(ap);

    cputs(buffer);

    return count;
}

int cprintfxy(int x, int y, const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, format, ap);
    va_end(ap);

    cputsxy(x, y, buffer);

    return count;
}

/* ============================================================================
 * KEYBOARD INPUT
 * ============================================================================ */

int getch(void) {
    /* TODO: Implement KERNAL keyboard input
     * For now, return placeholder
     */
    return 0;
}

int kbhit(void) {
    /* TODO: Implement KERNAL keyboard check
     * For now, return 0 (no key available)
     */
    return 0;
}

/* ============================================================================
 * SCREEN SIZE FUNCTIONS
 * ============================================================================ */

int screenwidth(void) {
    return screen_width;
}

int screenheight(void) {
    return screen_height;
}

/* ============================================================================
 * CURSOR VISIBILITY
 * ============================================================================ */

void cursoron(void) {
    /* TODO: Enable VIC-IV cursor
     * Modify VIC-IV register to show cursor
     */
}

void cursoroff(void) {
    /* TODO: Disable VIC-IV cursor
     * Modify VIC-IV register to hide cursor
     */
}

/* ============================================================================
 * SCREEN BUFFER FUNCTIONS
 * ============================================================================ */

int getch_xy(int x, int y) {
    int offset = _get_screen_offset(x, y);
    return (int)SCREEN_RAM[offset];
}

int getattr_xy(int x, int y) {
    int offset = _get_screen_offset(x, y);
    return (int)COLOR_RAM[offset];
}

