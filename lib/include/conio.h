/* conio.h — Console I/O for MEGA65 (Text Mode)
 *
 * cc65's conio, all of it (lib/stdlib_asm/conio.s45, in assembly; cprintf,
 * cprintfxy and cscanf in lib/stdlib/conio_printf.c), on the text screen the
 * VIC-IV shows: 80x25 in MEGA65 mode, 40 columns, 50 rows or a moved screen
 * too (screenwidth/screenheight read the VIC-IV). Text is PETSCII (what cc45
 * makes of C strings), shown as screen codes: '\n' starts the next row,
 * '\r' goes to column 0 (cc65's "\r\n" is one new row); other control
 * characters are not shown. Colour is per character (colour RAM); the
 * background is the screen's ($D021). The output scrolls at the bottom.
 *
 * The earlier cc45 names stay: putch is cputc, getch cgetc, textbackground
 * bgcolor, cursoron / cursoroff cursor(1) / cursor(0), screenwidth /
 * screenheight screensize's; cputch (x, y, ch) writes without moving the
 * cursor (cputcxy moves it).
 */

#pragma once

#include <stddef.h>
#include <stdarg.h>

/* ============================================================================
 * TEXT COLOR CONSTANTS
 * ============================================================================ */

#define BLACK           0
#define WHITE           1
#define RED             2
#define CYAN            3
#define MAGENTA         4
#define GREEN           5
#define BLUE            6
#define YELLOW          7
#define ORANGE          8
#define BROWN           9
#define LIGHT_RED       10
#define DARK_GRAY       11
#define MEDIUM_GRAY     12
#define LIGHT_GREEN     13
#define LIGHT_BLUE      14
#define LIGHT_GRAY      15

/* Additional MEGA65 colors (16-255 available in enhanced mode) */
#define PURPLE          4
#define GRAY            11

/* ============================================================================
 * SCREEN DIMENSIONS
 * ============================================================================ */

#define CONIO_COLS      80      /* MEGA65 mode's text screen (screenwidth() is the real one) */
#define CONIO_ROWS      25      /* (screenheight()) */

/* ============================================================================
 * LINE DRAWING CHARACTERS (PETSCII, as cc65's <cbm.h>)
 * ============================================================================ */

#ifndef CH_HLINE
#define CH_HLINE        192
#define CH_VLINE        221
#define CH_ULCORNER     176
#define CH_URCORNER     174
#define CH_LLCORNER     173
#define CH_LRCORNER     189
#define CH_TTEE         178
#define CH_BTEE         177
#define CH_LTEE         171
#define CH_RTEE         179
#define CH_CROSS        219
#endif
#ifndef CH_DEL
#define CH_DEL          20      /* DEL, as cgetc gives it */
#endif

/* ============================================================================
 * SCREEN AND CURSOR
 *
 * Positions are clamped to the screen. All __fastcall__ (the ZP convention in
 * either library).
 * ============================================================================ */

/* Clear the screen (spaces in the text colour) and home the cursor */
__fastcall__ void clrscr(void);

/* The cursor to (x, y), (0, 0) the top left corner */
__fastcall__ void gotoxy(int x, int y);
/* The cursor's column / row only */
__fastcall__ void gotox(unsigned char x);
__fastcall__ void gotoy(unsigned char y);

/* The cursor's column / row */
__fastcall__ int wherex(void);
__fastcall__ int wherey(void);

/* The screen's columns and rows (the VIC-IV's $D05E and $D07B + 1) */
__fastcall__ void screensize(unsigned char *x, unsigned char *y);
__fastcall__ int screenwidth(void);
__fastcall__ int screenheight(void);

/* ============================================================================
 * COLOURS AND ATTRIBUTES
 * ============================================================================ */

/* The text colour (0-15) from now on; the old one */
__fastcall__ unsigned char textcolor(unsigned char color);

/* The screen's background ($D021: text mode has no background per
 * character; the full-colour mode below has); the old one */
__fastcall__ unsigned char bgcolor(unsigned char color);
/* bgcolor */
__fastcall__ unsigned char textbackground(int color);

/* The border ($D020); the old one */
__fastcall__ unsigned char bordercolor(unsigned char color);

/* textcolor(attr & 15) and the background attr >> 4 */
__fastcall__ void textattr(int attr);

/* Reversed characters from now on (onoff 1) or not (0); the old setting */
__fastcall__ unsigned char revers(unsigned char onoff);

/* ============================================================================
 * OUTPUT (at the cursor, which advances: wrapping, scrolling at the bottom)
 * ============================================================================ */

/* A character */
__fastcall__ void cputc(char c);
/* cputc, returning ch */
__fastcall__ int putch(int ch);
/* gotoxy(x, y), then cputc(c) */
__fastcall__ void cputcxy(unsigned char x, unsigned char y, char c);
/* ch at (x, y); the cursor stays */
__fastcall__ void cputch(int x, int y, int ch);

/* A string: 0, or -1 for NULL */
__fastcall__ int cputs(const char *str);
/* gotoxy(x, y), then cputs(str) */
__fastcall__ int cputsxy(int x, int y, const char *str);

/* printf to the cursor (no length limit; %f %e %g as printf's); the
 * number of characters */
int cprintf(const char *format, ...);
__fastcall__ int vcprintf(const char *format, va_list ap);
/* gotoxy(x, y), then cprintf */
int cprintfxy(int x, int y, const char *format, ...);

/* Two / four hex digits */
__fastcall__ void cputhex8(unsigned char val);
__fastcall__ void cputhex16(unsigned val);

/* A horizontal line (CH_HLINE) of length characters from the cursor */
__fastcall__ void chline(unsigned char length);
__fastcall__ void chlinexy(unsigned char x, unsigned char y, unsigned char length);
/* A vertical line (CH_VLINE) down from the cursor (to the bottom row at
 * most); the cursor ends below it */
__fastcall__ void cvline(unsigned char length);
__fastcall__ void cvlinexy(unsigned char x, unsigned char y, unsigned char length);
/* length spaces from the cursor */
__fastcall__ void cclear(unsigned char length);
__fastcall__ void cclearxy(unsigned char x, unsigned char y, unsigned char length);

/* ============================================================================
 * KEYBOARD
 * ============================================================================ */

/* The next key, waiting for one, without echo: <input.h>'s codes (PETSCII
 * letters, RETURN 13, DEL 20, RUN/STOP 3, F1-F14 $F1-$FE). With cursor(1)
 * the cursor cell shows reversed while it waits. */
__fastcall__ char cgetc(void);
/* cgetc */
__fastcall__ int getch(void);

/* 1 when a key is waiting (in the KERNAL's buffer or the keyboard's queue
 * at $D610) */
__fastcall__ int kbhit(void);

/* cgetc shows the cursor (onoff 1) or not (0, the default); the old setting */
__fastcall__ unsigned char cursor(unsigned char onoff);
__fastcall__ void cursoron(void);
__fastcall__ void cursoroff(void);

/* A line typed at the cursor, echoed with the cursor shown: up to size - 1
 * characters, or until RETURN (echoed as a new row, not stored); DEL takes
 * the last character back. buffer, or NULL when nothing was typed (or for
 * a NULL buffer or a size below 2), as cc65's. */
__fastcall__ char *cgets(char *buffer, int size);

/* scanf of a line typed as cgets reads it (80 characters at most); what the
 * format does not use is dropped. The items assigned, or EOF (-1). */
int cscanf(const char *format, ...);
__fastcall__ int vcscanf(const char *format, va_list ap);

/* ============================================================================
 * READING THE SCREEN
 * ============================================================================ */

/* The character at the cursor (PETSCII, as cputc writes it), its colour,
 * and whether it is reversed (1) */
__fastcall__ char cpeekc(void);
__fastcall__ unsigned char cpeekcolor(void);
__fastcall__ unsigned char cpeekrevers(void);

/* length characters from the cursor on (into the next row after the last
 * column; none past the bottom), then a NUL; the cursor stays */
__fastcall__ void cpeeks(char *s, unsigned int length);

/* The screen code at (x, y) (with its reverse bit) */
__fastcall__ int getch_xy(int x, int y);
/* The colour RAM byte at (x, y) */
__fastcall__ int getattr_xy(int x, int y);

/* ============================================================================
 * FULL-COLOUR TEXT MODE: A BACKGROUND COLOUR FOR EACH CHARACTER
 *
 * The VIC-IV's text mode has one background colour, the screen's ($D021).
 * fcm_enable makes the screen two bytes a cell (CHR16) with full-colour
 * characters for character numbers from 256 (FCLRHI): 64 bytes, a byte a
 * pixel, at the number * 64. A cell on the screen's background shows the
 * font's character; one on another background a glyph made from the font,
 * its clear pixels in that colour, kept in a cache (256 glyphs, 64 bytes
 * each, in bank 4 or 5 through far_malloc, after the screen: about 24 KB at
 * 80x50). When the cache is full the glyphs no cell shows are dropped; when
 * all are on the screen, a new cell is drawn on the screen's background.
 * The other conio functions keep working (on the screen's background);
 * stdio output through the ROM goes to the ROM's screen, not shown until
 * fcm_disable. Colours are 0-15; a black (0) background uses palette entry
 * $FE, which fcm_enable sets to black.
 * ============================================================================ */

/* Enable it, keeping what is on the screen. 1, or 0 when far_malloc finds
 * no room in banks 4 and 5. */
__fastcall__ int fcm_enable(void);

/* Back to the screen as it was (one byte a cell), with what is on it; the
 * cache's memory is freed. */
__fastcall__ void fcm_disable(void);

/* 1 while enabled */
__fastcall__ int fcm_is_enabled(void);

/* The cache's size (1-256 glyphs) for the next fcm_enable; 0 while enabled
 * or out of range. */
__fastcall__ int fcm_set_cache_size(int glyphs);

/* PETSCII character ch at (x, y) in colour fg on background bg (without
 * fcm_enable, on the screen's background). The cursor stays; revers applies. */
__fastcall__ void fcm_putch(int x, int y, int ch, int fg, int bg);

/* A string from (x, y), wrapping at the right edge, '\n' starting the next
 * row; it stops at the bottom. 0, or -1 for NULL. */
__fastcall__ int fcm_cputs(int x, int y, const char *str, int fg, int bg);

/* fcm_cputs of printf-formatted text (up to 255 characters); the count
 * vsnprintf gives */
int fcm_cprintf(int x, int y, int fg, int bg, const char *format, ...);

/* The rectangle (x1, y1)-(x2, y2), corners included and clipped to the
 * screen, filled with ch in fg on bg */
__fastcall__ void fcm_fill_rect(int x1, int y1, int x2, int y2, int ch, int fg, int bg);

/* Row y's characters recoloured: fg on bg */
__fastcall__ void fcm_set_line_color(int y, int fg, int bg);

/* (bg << 4) | fg at (x, y) */
__fastcall__ int fcm_getattr_xy(int x, int y);

/* The program's own full-colour character n (256 or more: its 64 bytes, a
 * byte a pixel, at n * 64 in the first 512 KB of chip RAM, such as bank 4
 * through far_malloc; pixel $00 the screen's background, $FF colour fg,
 * others that palette entry) at (x, y); nothing without fcm_enable.
 * getch_xy / fcm_getattr_xy do not know it; fcm_disable leaves a space. */
__fastcall__ void fcm_putglyph(int x, int y, unsigned int n, int fg);

/* ============================================================================
 * SCREEN WIDTH
 * ============================================================================ */

/* 40 or 80 columns (H640, $D031 bit 7; with fcm_enable too): the screen's
 * address is kept and the screen cleared. 1, or 0 for another width. The
 * ROM's screen editor is not told: set the width back before returning to
 * BASIC. */
__fastcall__ int conio_set_width(int width);

/* screenwidth() */
__fastcall__ int conio_get_width(void);

