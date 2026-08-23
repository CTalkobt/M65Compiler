/* conio.h — Console I/O for MEGA65 (Text Mode)
 *
 * Provides conio-compatible console I/O functions for text mode.
 * Compatible with standard C compiler conio libraries (cc65, etc.)
 */

#pragma once

#include <stddef.h>

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

#define CONIO_COLS      40      /* Standard 40-column text mode */
#define CONIO_ROWS      25      /* Standard 25-row text mode */

/* ============================================================================
 * SCREEN CONTROL FUNCTIONS
 * ============================================================================ */

/**
 * clrscr - Clear screen and home cursor
 *
 * Clears the entire screen with the current background color
 * and positions the cursor at (0, 0).
 */
void clrscr(void);

/**
 * gotoxy - Position cursor
 *
 * Moves the cursor to the specified (x, y) position.
 * (0, 0) is the top-left corner.
 *
 * Parameters:
 *   x — Column (0-39 for 40-column mode)
 *   y — Row (0-24 for 25-row mode)
 */
void gotoxy(int x, int y);

/**
 * wherex - Get cursor column
 *
 * Returns the current cursor column (0-39).
 */
int wherex(void);

/**
 * wherey - Get cursor row
 *
 * Returns the current cursor row (0-24).
 */
int wherey(void);

/* ============================================================================
 * TEXT ATTRIBUTE FUNCTIONS
 * ============================================================================ */

/**
 * textcolor - Set text foreground color
 *
 * Sets the color for subsequent text output.
 *
 * Parameters:
 *   color — Color index (0-15 standard, 0-255 in enhanced mode)
 */
void textcolor(int color);

/**
 * textbackground - Set text background color
 *
 * Sets the background color for subsequent text output.
 *
 * Parameters:
 *   color — Background color index (0-15)
 */
void textbackground(int color);

/**
 * textattr - Set combined text attributes
 *
 * Sets both foreground and background color at once.
 * attr = (background << 4) | foreground
 *
 * Parameters:
 *   attr — Combined attribute byte
 */
void textattr(int attr);

/* ============================================================================
 * CHARACTER OUTPUT
 * ============================================================================ */

/**
 * putch - Write character at cursor position
 *
 * Writes a character at the current cursor position
 * and advances the cursor.
 *
 * Parameters:
 *   ch — Character to write
 *
 * Returns:
 *   The character written
 */
int putch(int ch);

/**
 * cputch - Write character with explicit position
 *
 * Writes a character at position (x, y).
 *
 * Parameters:
 *   x — Column
 *   y — Row
 *   ch — Character to write
 */
void cputch(int x, int y, int ch);

/**
 * cputs - Write string at cursor position
 *
 * Writes a string starting at the current cursor position.
 *
 * Parameters:
 *   str — Null-terminated string
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cputs(const char *str);

/**
 * cputsxy - Write string at explicit position
 *
 * Writes a string starting at position (x, y).
 *
 * Parameters:
 *   x — Column
 *   y — Row
 *   str — Null-terminated string
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cputsxy(int x, int y, const char *str);

/* ============================================================================
 * FORMATTED OUTPUT
 * ============================================================================ */

/**
 * cprintf - Formatted console output
 *
 * Like printf, but outputs to current cursor position.
 *
 * Parameters:
 *   format — Format string (printf-compatible)
 *   ... — Variable arguments
 *
 * Returns:
 *   Number of characters written
 */
int cprintf(const char *format, ...);

/**
 * cprintfxy - Formatted output at explicit position
 *
 * Like cprintf, but outputs to position (x, y).
 *
 * Parameters:
 *   x — Column
 *   y — Row
 *   format — Format string
 *   ... — Variable arguments
 *
 * Returns:
 *   Number of characters written
 */
int cprintfxy(int x, int y, const char *format, ...);

/* ============================================================================
 * KEYBOARD INPUT
 * ============================================================================ */

/**
 * getch - Read character from keyboard (blocking)
 *
 * Waits for a key press and returns the character code.
 *
 * Returns:
 *   Character code (1-127 for ASCII, 128+ for special keys)
 */
int getch(void);

/**
 * kbhit - Check if key is available (non-blocking)
 *
 * Returns whether a key press is available without blocking.
 *
 * Returns:
 *   1 if key available, 0 if no key
 */
int kbhit(void);

/* ============================================================================
 * SCREEN SIZE FUNCTIONS
 * ============================================================================ */

/**
 * screenwidth - Get screen width in columns
 *
 * Returns:
 *   Width (40 for standard mode, 80 for enhanced)
 */
int screenwidth(void);

/**
 * screenheight - Get screen height in rows
 *
 * Returns:
 *   Height (typically 25)
 */
int screenheight(void);

/* ============================================================================
 * CURSOR VISIBILITY
 * ============================================================================ */

/**
 * cursoron - Make cursor visible
 */
void cursoron(void);

/**
 * cursoroff - Hide cursor
 */
void cursoroff(void);

/* ============================================================================
 * SCREEN BUFFER FUNCTIONS
 * ============================================================================ */

/**
 * getch_xy - Read character from screen position
 *
 * Reads the character currently displayed at position (x, y).
 *
 * Parameters:
 *   x — Column
 *   y — Row
 *
 * Returns:
 *   Character code
 */
int getch_xy(int x, int y);

/**
 * getattr_xy - Read attribute from screen position
 *
 * Reads the color attribute at position (x, y).
 *
 * Parameters:
 *   x — Column
 *   y — Row
 *
 * Returns:
 *   Attribute byte (background << 4 | foreground)
 */
int getattr_xy(int x, int y);

