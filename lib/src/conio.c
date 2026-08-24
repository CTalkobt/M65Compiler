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

/* Full-Color Text Mode state */
static int fcm_enabled = 0;
static unsigned char *fcm_attr_ram = (unsigned char *)0x2000;  /* Attribute RAM (alt location) */

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

/* ============================================================================
 * FULL-COLOR TEXT MODE IMPLEMENTATION
 * ========================================================================== */

int fcm_enable(void) {
    /* Check if FCM is available (Phase 30 integration point) */
    /* For now, always succeed - real implementation checks FEATURE_FCM */

    if (fcm_enabled) return 1;

    /* Unlock VIC-III/IV */
    vic4->key = 0xA5;
    vic4->key = 0x96;

    /* Enable full-color mode in VIC-IV control B */
    vic4->ctrl_b |= VIC4_FCM;

    /* Enable 16-bit character mode in VIC-IV control C */
    vic4->ctrl_c |= VIC4_CHR16;

    /* Clear attribute RAM (set all backgrounds to black) */
    int i;
    int size = screen_width * screen_height;
    for (i = 0; i < size; ++i) {
        fcm_attr_ram[i] = 0;
    }

    fcm_enabled = 1;
    return 1;
}

void fcm_disable(void) {
    if (!fcm_enabled) return;

    /* Unlock VIC-III/IV */
    vic4->key = 0xA5;
    vic4->key = 0x96;

    /* Disable full-color mode */
    vic4->ctrl_b &= ~VIC4_FCM;

    /* Disable 16-bit character mode */
    vic4->ctrl_c &= ~VIC4_CHR16;

    fcm_enabled = 0;
}

int fcm_is_enabled(void) {
    return fcm_enabled;
}

void fcm_putch(int x, int y, int ch, int fg, int bg) {
    int offset = _get_screen_offset(x, y);

    SCREEN_RAM[offset] = (unsigned char)ch;

    if (fcm_enabled) {
        /* In FCM mode: foreground in COLOR_RAM, background in ATTR_RAM */
        COLOR_RAM[offset] = (unsigned char)fg;
        fcm_attr_ram[offset] = (unsigned char)bg;
    } else {
        /* In standard mode: combined (bg << 4) | fg */
        COLOR_RAM[offset] = (unsigned char)((bg << 4) | fg);
    }
}

int fcm_cputs(int x, int y, const char *str, int fg, int bg) {
    int count = 0;
    int cx = x;
    int cy = y;

    if (!str) return -1;

    while (*str) {
        fcm_putch(cx, cy, (int)*str, fg, bg);
        count++;
        cx++;

        if (cx >= screen_width) {
            cx = 0;
            cy++;
            if (cy >= screen_height) {
                break;  /* Reached end of screen */
            }
        }

        str++;
    }

    return 0;
}

int fcm_cprintf(int x, int y, int fg, int bg, const char *format, ...) {
    char buffer[256];
    va_list args;
    int count;

    va_start(args, format);
    count = vsprintf(buffer, format, args);
    va_end(args);

    if (count >= 0) {
        fcm_cputs(x, y, buffer, fg, bg);
    }

    return count;
}

void fcm_fill_rect(int x1, int y1, int x2, int y2, int ch, int fg, int bg) {
    int x, y;

    for (y = y1; y <= y2 && y < screen_height; ++y) {
        for (x = x1; x <= x2 && x < screen_width; ++x) {
            fcm_putch(x, y, ch, fg, bg);
        }
    }
}

void fcm_set_line_color(int y, int fg, int bg) {
    int x;

    if (y < 0 || y >= screen_height) return;

    for (x = 0; x < screen_width; ++x) {
        int offset = _get_screen_offset(x, y);

        if (fcm_enabled) {
            COLOR_RAM[offset] = (unsigned char)fg;
            fcm_attr_ram[offset] = (unsigned char)bg;
        } else {
            COLOR_RAM[offset] = (unsigned char)((bg << 4) | fg);
        }
    }
}

int fcm_getattr_xy(int x, int y) {
    int offset = _get_screen_offset(x, y);

    if (fcm_enabled) {
        int fg = (int)COLOR_RAM[offset];
        int bg = (int)fcm_attr_ram[offset];
        return (bg << 4) | fg;
    } else {
        return (int)COLOR_RAM[offset];
    }
}

int conio_set_width(int width) {
    if (width == 40) {
        /* Reset to 40-column mode */
        vic4->key = 0xA5;
        vic4->key = 0x96;
        vic4->ctrl_c &= ~VIC4_H640;

        screen_width = CONIO_COLS;
        return 1;
    } else if (width == 80) {
        /* Switch to 80-column H640 mode */
        vic4->key = 0xA5;
        vic4->key = 0x96;
        vic4->ctrl_c |= VIC4_H640;

        screen_width = 80;
        return 1;
    }

    return 0;
}

int conio_get_width(void) {
    return screen_width;
}

