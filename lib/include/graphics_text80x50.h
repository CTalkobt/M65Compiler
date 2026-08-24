/* graphics_text80x50.h — TEXT_80x50 Graphics Mode (Phase 6)
 *
 * 80×50 extended text mode using MEGA65 hardware capabilities.
 * Finer grid than standard 40×25 for more detailed text-based graphics.
 */

#pragma once

#include <graphics_hal.h>

/* ============================================================================
 * TEXT_80x50 MODE DEFINITION (Phase 6)
 * ============================================================================ */

/**
 * TEXT_80x50 Graphics Mode
 *
 * - 80 characters wide × 50 rows tall
 * - Single-byte characters with 4-bit color (16 colors)
 * - Uses 4KB screen memory for character data
 * - Uses 4KB color memory for per-character color
 * - Hardware-accelerated via MEGA65 VIC-IV
 * - Supports all standard text primitives
 */

/**
 * Initialize TEXT_80x50 mode
 *
 * Sets up VIC-IV for 80×50 text mode with proper register configuration.
 */
void text80x50_init(void);

/**
 * Cleanup TEXT_80x50 mode
 */
void text80x50_done(void);

/**
 * Set character at position
 *
 * Parameters:
 *   x — Column (0-79)
 *   y — Row (0-49)
 *   ch — Character to display
 *   color — Color (0-15)
 */
void text80x50_putchar(int x, int y, unsigned char ch, unsigned char color);

/**
 * Get character at position
 *
 * Parameters:
 *   x — Column (0-79)
 *   y — Row (0-49)
 *
 * Returns:
 *   Character at position, or 0 if out of bounds
 */
unsigned char text80x50_getchar(int x, int y);

/**
 * Get color at position
 *
 * Returns:
 *   Color (0-15) at position, or 0 if out of bounds
 */
unsigned char text80x50_getcolor(int x, int y);

/**
 * Set color at position (without changing character)
 */
void text80x50_setcolor(int x, int y, unsigned char color);

/**
 * Clear screen with character and color
 *
 * Parameters:
 *   ch — Fill character (usually space 0x20)
 *   color — Fill color (0-15)
 */
void text80x50_clear(unsigned char ch, unsigned char color);

/**
 * Set foreground color for text operations
 */
void text80x50_setfgcolor(unsigned char color);

/**
 * Set background color for text operations
 */
void text80x50_setbgcolor(unsigned char color);

/**
 * Get current foreground color
 */
unsigned char text80x50_getfgcolor(void);

/**
 * Get current background color
 */
unsigned char text80x50_getbgcolor(void);

/* ============================================================================
 * DRAWING PRIMITIVES (Phase 6)
 * ============================================================================ */

/**
 * Draw horizontal line
 *
 * Parameters:
 *   x1, y — Starting position
 *   x2 — Ending column
 *   ch — Character to use
 *   color — Line color
 */
void text80x50_hline(int x1, int y, int x2, unsigned char ch, unsigned char color);

/**
 * Draw vertical line
 *
 * Parameters:
 *   x — Column
 *   y1 — Starting row
 *   y2 — Ending row
 *   ch — Character to use
 *   color — Line color
 */
void text80x50_vline(int x, int y1, int y2, unsigned char ch, unsigned char color);

/**
 * Draw rectangle outline
 *
 * Parameters:
 *   x1, y1 — Top-left corner
 *   x2, y2 — Bottom-right corner
 *   ch — Border character
 *   color — Border color
 */
void text80x50_rect(int x1, int y1, int x2, int y2, unsigned char ch, unsigned char color);

/**
 * Draw filled rectangle
 *
 * Parameters:
 *   x1, y1 — Top-left corner
 *   x2, y2 — Bottom-right corner
 *   ch — Fill character
 *   color — Fill color
 */
void text80x50_fillrect(int x1, int y1, int x2, int y2, unsigned char ch, unsigned char color);

/**
 * Print string at position
 *
 * Parameters:
 *   x, y — Starting position
 *   str — Null-terminated string
 *   color — Text color
 *
 * Returns:
 *   Ending x position after printing
 */
int text80x50_print(int x, int y, const char *str, unsigned char color);

/**
 * Draw box with border
 *
 * Parameters:
 *   x1, y1 — Top-left corner
 *   x2, y2 — Bottom-right corner
 *   border_ch — Border character
 *   fill_ch — Fill character
 *   border_color — Border color
 *   fill_color — Fill color
 */
void text80x50_box(int x1, int y1, int x2, int y2,
                   unsigned char border_ch, unsigned char fill_ch,
                   unsigned char border_color, unsigned char fill_color);

/* ============================================================================
 * CURSOR & INPUT (Phase 6)
 * ============================================================================ */

/**
 * Show/hide text cursor
 *
 * Parameters:
 *   x, y — Cursor position
 *   visible — 1 = show, 0 = hide
 */
void text80x50_cursor(int x, int y, int visible);

/**
 * Get cursor position
 */
void text80x50_get_cursor(int *x, int *y);

/**
 * Set cursor position
 */
void text80x50_set_cursor(int x, int y);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define TEXT80X50_WIDTH         80      /* Screen width in characters */
#define TEXT80X50_HEIGHT        50      /* Screen height in rows */
#define TEXT80X50_SCREEN_ADDR   0x0800  /* Screen memory start address */
#define TEXT80X50_COLOR_ADDR    0x2800  /* Color memory start address */
#define TEXT80X50_BUFFER_SIZE   (80 * 50)  /* Total character capacity */

/* Standard colors (0-15) */
#define COLOR_BLACK             0
#define COLOR_WHITE             1
#define COLOR_RED               2
#define COLOR_CYAN              3
#define COLOR_MAGENTA           4
#define COLOR_GREEN             5
#define COLOR_BLUE              6
#define COLOR_YELLOW            7
#define COLOR_ORANGE            8
#define COLOR_BROWN             9
#define COLOR_PINK              10
#define COLOR_GRAY1             11
#define COLOR_GRAY2             12
#define COLOR_LTGREEN           13
#define COLOR_LTBLUE            14
#define COLOR_GRAY3             15
