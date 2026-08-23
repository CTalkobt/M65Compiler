/* graphics.h — MEGA65 Graphics Framework
 *
 * Device-independent graphics API with pluggable drivers.
 * Inspired by cc65's TGI but optimized for MEGA65 hardware.
 *
 * Phase 1: Core framework (driver abstraction, mode switching)
 */

#pragma once

#include <graphics_hal.h>

/* ============================================================================
 * GRAPHICS MODES
 * ============================================================================ */

typedef enum {
    GRAPHICS_MODE_TEXT_40x25,       /* 40×25 text mode (standard) */
    GRAPHICS_MODE_TEXT_80x24,       /* 80×24 text mode (MEGA65 extended) */
    GRAPHICS_MODE_BITMAP_320x200,   /* 320×200 monochrome bitmap */
    GRAPHICS_MODE_BITMAP_160x200    /* 160×200 multicolor bitmap */
} graphics_mode_t;

/* ============================================================================
 * DRIVER STRUCTURE
 * ============================================================================ */

typedef struct graphics_driver {
    const char *name;               /* Driver name (e.g., "BITMAP_320x200") */
    graphics_mode_t mode;           /* Graphics mode this driver implements */
    int width;                      /* Screen width in pixels/chars */
    int height;                     /* Screen height in pixels/rows */
    int max_colors;                 /* Maximum colors (16, 256, etc.) */

    /* Lifecycle */
    void (*init)(void);             /* Initialize mode */
    void (*done)(void);             /* Cleanup mode */

    /* Color & state */
    void (*setcolor)(unsigned char c);      /* Set foreground color */
    void (*setbkcolor)(unsigned char c);    /* Set background color */

    /* Drawing primitives */
    void (*plot)(int x, int y);             /* Plot single pixel */
    unsigned char (*getpixel)(int x, int y);/* Read pixel */
    void (*line)(int x1, int y1, int x2, int y2);      /* Draw line */
    void (*bar)(int x1, int y1, int x2, int y2);       /* Filled rectangle */
    void (*rect)(int x1, int y1, int x2, int y2);      /* Rectangle outline */
    void (*circle)(int x, int y, int radius);          /* Circle */
    void (*clear)(void);            /* Clear screen */

    /* Optional: text, sprites, patterns (for Phase 2+) */
} graphics_driver_t;

/* ============================================================================
 * FRAMEWORK STATE
 * ============================================================================ */

typedef struct {
    graphics_driver_t *active_driver;       /* Currently active driver */
    unsigned char current_color;            /* Current foreground color */
    unsigned char current_bkcolor;          /* Current background color */
    int clip_x1, clip_y1, clip_x2, clip_y2; /* Clipping rectangle */
} graphics_state_t;

extern graphics_state_t graphics_state;

/* ============================================================================
 * FRAMEWORK INITIALIZATION
 * ============================================================================ */

/**
 * graphics_init - Initialize graphics framework
 *
 * Sets up default mode and driver. Must be called before any graphics ops.
 */
void graphics_init(void);

/**
 * graphics_done - Cleanup graphics framework
 *
 * Restores hardware to default state.
 */
void graphics_done(void);

/* ============================================================================
 * MODE MANAGEMENT
 * ============================================================================ */

/**
 * graphics_setmode - Switch to specified graphics mode
 *
 * Loads driver for mode and initializes hardware.
 *
 * Parameters:
 *   mode — Target graphics mode (GRAPHICS_MODE_*)
 *
 * Returns:
 *   0 on success, -1 on error (mode not supported)
 */
int graphics_setmode(graphics_mode_t mode);

/**
 * graphics_getmode - Get current graphics mode
 *
 * Returns:
 *   Current mode enum
 */
graphics_mode_t graphics_getmode(void);

/**
 * graphics_getwidth - Get screen width
 *
 * Returns:
 *   Width in pixels (bitmap) or characters (text)
 */
int graphics_getwidth(void);

/**
 * graphics_getheight - Get screen height
 *
 * Returns:
 *   Height in pixels (bitmap) or rows (text)
 */
int graphics_getheight(void);

/**
 * graphics_getmaxcolor - Get maximum color index
 *
 * Returns:
 *   Max color value (15 for 16-color, 255 for 256-color, etc.)
 */
int graphics_getmaxcolor(void);

/* ============================================================================
 * COLOR CONTROL
 * ============================================================================ */

/**
 * graphics_setcolor - Set foreground color
 *
 * Parameters:
 *   color — Color index (0-15 for standard, 0-255 for extended)
 */
void graphics_setcolor(unsigned char color);

/**
 * graphics_setbkcolor - Set background color
 *
 * Parameters:
 *   color — Background color index
 */
void graphics_setbkcolor(unsigned char color);

/**
 * graphics_getcolor - Get current foreground color
 *
 * Returns:
 *   Current foreground color
 */
unsigned char graphics_getcolor(void);

/**
 * graphics_getbkcolor - Get current background color
 *
 * Returns:
 *   Current background color
 */
unsigned char graphics_getbkcolor(void);

/* ============================================================================
 * DRAWING PRIMITIVES
 * ============================================================================ */

/**
 * graphics_plot - Plot single pixel
 *
 * Parameters:
 *   x, y — Pixel coordinates
 */
void graphics_plot(int x, int y);

/**
 * graphics_getpixel - Read pixel value
 *
 * Parameters:
 *   x, y — Pixel coordinates
 *
 * Returns:
 *   Pixel color value
 */
unsigned char graphics_getpixel(int x, int y);

/**
 * graphics_line - Draw line
 *
 * Bresenham line algorithm from (x1,y1) to (x2,y2).
 *
 * Parameters:
 *   x1, y1, x2, y2 — Line endpoints
 */
void graphics_line(int x1, int y1, int x2, int y2);

/**
 * graphics_bar - Draw filled rectangle
 *
 * Parameters:
 *   x1, y1, x2, y2 — Rectangle corners (top-left to bottom-right)
 */
void graphics_bar(int x1, int y1, int x2, int y2);

/**
 * graphics_rect - Draw rectangle outline
 *
 * Parameters:
 *   x1, y1, x2, y2 — Rectangle corners
 */
void graphics_rect(int x1, int y1, int x2, int y2);

/**
 * graphics_circle - Draw circle
 *
 * Midpoint circle algorithm.
 *
 * Parameters:
 *   x, y — Center coordinates
 *   radius — Circle radius in pixels
 */
void graphics_circle(int x, int y, int radius);

/**
 * graphics_clear - Clear entire screen
 *
 * Fills screen with background color.
 */
void graphics_clear(void);

/* ============================================================================
 * CLIPPING
 * ============================================================================ */

/**
 * graphics_setclip - Set clipping rectangle
 *
 * All drawing operations clipped to this region.
 *
 * Parameters:
 *   x1, y1, x2, y2 — Clip rectangle corners
 */
void graphics_setclip(int x1, int y1, int x2, int y2);

/**
 * graphics_clearclip - Disable clipping
 *
 * Sets clip region to full screen.
 */
void graphics_clearclip(void);

/**
 * graphics_getclip - Get current clipping rectangle
 *
 * Parameters:
 *   x1, y1, x2, y2 — Output: clip rectangle
 */
void graphics_getclip(int *x1, int *y1, int *x2, int *y2);

