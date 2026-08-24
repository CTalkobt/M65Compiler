/* palette.h — Color Palette Management for MEGA65
 *
 * Provides palette control functions for VIC-IV, including bank selection,
 * color loading, fade effects, and palette interpolation.
 *
 * Works with Phase 31 (FCM) to provide dynamic color control.
 * Supports standard C64 16-color palette plus extended RGB modes.
 *
 * Usage:
 *   #include <palette.h>
 *   palette_load_standard();        // Load C64 palette
 *   palette_set_color(RED, 255, 0, 0); // Change a color
 *   palette_fade_in(10);            // Fade in effect
 */

#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

/* ============================================================================
 * COLOR STRUCTURE
 * ========================================================================== */

typedef struct {
    uint8_t r;  /* Red component (0-255) */
    uint8_t g;  /* Green component (0-255) */
    uint8_t b;  /* Blue component (0-255) */
} rgb_color_t;

/* ============================================================================
 * PALETTE STRUCTURE
 * ========================================================================== */

typedef struct {
    rgb_color_t colors[16];  /* 16 colors per palette */
} palette_t;

/* ============================================================================
 * PALETTE CONSTANTS (C64 Standard Colors)
 * ========================================================================== */

#define PAL_BLACK       0
#define PAL_WHITE       1
#define PAL_RED         2
#define PAL_CYAN        3
#define PAL_MAGENTA     4
#define PAL_GREEN       5
#define PAL_BLUE        6
#define PAL_YELLOW      7
#define PAL_ORANGE      8
#define PAL_BROWN       9
#define PAL_LIGHT_RED   10
#define PAL_DARK_GRAY   11
#define PAL_MEDIUM_GRAY 12
#define PAL_LIGHT_GREEN 13
#define PAL_LIGHT_BLUE  14
#define PAL_LIGHT_GRAY  15

/* ============================================================================
 * PALETTE BANK ENUMERATION
 * ========================================================================== */

typedef enum {
    PALETTE_BANK_0 = 0,
    PALETTE_BANK_1 = 1,
    PALETTE_BANK_2 = 2,
    PALETTE_BANK_3 = 3,
    /* Up to 16 banks available on MEGA65 */
} palette_bank_t;

/* ============================================================================
 * PALETTE SELECTION & CONTROL
 * ========================================================================== */

/**
 * palette_select_bank - Switch active palette bank
 *
 * Selects which palette bank (0-15) is active for display.
 *
 * Parameters:
 *   bank — Palette bank number (0-15)
 *
 * Returns:
 *   1 if successful, 0 if bank unavailable
 */
int palette_select_bank(palette_bank_t bank);

/**
 * palette_get_bank - Get currently active palette bank
 *
 * Returns:
 *   Currently selected palette bank (0-15)
 */
palette_bank_t palette_get_bank(void);

/**
 * palette_get_color - Read current color
 *
 * Reads the RGB value of a color index in the active palette.
 *
 * Parameters:
 *   index — Color index (0-15)
 *   out_color — Pointer to rgb_color_t to receive the color
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int palette_get_color(int index, rgb_color_t *out_color);

/**
 * palette_set_color - Set a palette color
 *
 * Changes a color value in the active palette.
 *
 * Parameters:
 *   index — Color index (0-15)
 *   r, g, b — RGB components (0-255)
 */
void palette_set_color(int index, uint8_t r, uint8_t g, uint8_t b);

/**
 * palette_set_color_rgb - Set color from rgb_color_t
 *
 * Parameters:
 *   index — Color index (0-15)
 *   color — RGB color structure
 */
void palette_set_color_rgb(int index, rgb_color_t color);

/* ============================================================================
 * PALETTE LOADING & PRESETS
 * ========================================================================== */

/**
 * palette_load_standard - Load standard C64 palette
 *
 * Loads the classic 16-color Commodore 64 palette into the active bank.
 */
void palette_load_standard(void);

/**
 * palette_load_grayscale - Load grayscale palette
 *
 * Creates a grayscale palette with 16 shades from black to white.
 */
void palette_load_grayscale(void);

/**
 * palette_load_custom - Load custom palette from array
 *
 * Loads a full 16-color palette from an rgb_color_t array.
 *
 * Parameters:
 *   colors — Array of 16 rgb_color_t values
 */
void palette_load_custom(const rgb_color_t colors[16]);

/**
 * palette_load_by_name - Load preset palette by name
 *
 * Loads a built-in palette by name. Names include:
 *   - "standard" — C64 palette
 *   - "grayscale" — 16-shade gray
 *   - "dos" — DOS 16-color palette
 *   - "apple2" — Apple II style
 *
 * Parameters:
 *   name — Palette name (null-terminated string)
 *
 * Returns:
 *   1 if palette found and loaded, 0 if not found
 */
int palette_load_by_name(const char *name);

/* ============================================================================
 * PALETTE EFFECTS & TRANSFORMATIONS
 * ========================================================================== */

/**
 * palette_fade_in - Fade in effect from black
 *
 * Gradually increases all colors from black (0,0,0) to their
 * target values over multiple frames.
 *
 * Parameters:
 *   speed — Number of frames per fade step (1-20)
 *           Lower = faster fade
 */
void palette_fade_in(int speed);

/**
 * palette_fade_out - Fade out effect to black
 *
 * Gradually decreases all colors to black over multiple frames.
 *
 * Parameters:
 *   speed — Number of frames per fade step (1-20)
 */
void palette_fade_out(int speed);

/**
 * palette_rotate - Rotate color indices
 *
 * Rotates a range of colors within the palette.
 * Useful for animation (e.g., scrolling fire effect).
 *
 * Parameters:
 *   start — First color index to rotate
 *   end — Last color index to rotate
 *   direction — 1 for forward, -1 for backward
 */
void palette_rotate(int start, int end, int direction);

/**
 * palette_interpolate - Blend two palettes
 *
 * Creates an intermediate palette between source and target.
 * Useful for palette transitions.
 *
 * Parameters:
 *   src — Source palette
 *   dst — Destination palette
 *   factor — Blend factor (0.0 = src, 1.0 = dst, 0.5 = 50/50)
 *   out — Output palette
 */
void palette_interpolate(const palette_t *src, const palette_t *dst,
                         float factor, palette_t *out);

/* ============================================================================
 * PALETTE UTILITY FUNCTIONS
 * ========================================================================== */

/**
 * palette_invert_color - Invert a color (bitwise NOT on RGB)
 *
 * Parameters:
 *   color — Input color
 *
 * Returns:
 *   Inverted color
 */
rgb_color_t palette_invert_color(rgb_color_t color);

/**
 * palette_desaturate - Convert color to grayscale
 *
 * Parameters:
 *   color — Input color
 *
 * Returns:
 *   Grayscale version (R=G=B=luminance)
 */
rgb_color_t palette_desaturate(rgb_color_t color);

/**
 * palette_brighten - Increase color brightness
 *
 * Parameters:
 *   color — Input color
 *   factor — Brightness multiplier (0.0-2.0)
 *
 * Returns:
 *   Brightened color (clamped to 255)
 */
rgb_color_t palette_brighten(rgb_color_t color, float factor);

/**
 * palette_darken - Decrease color brightness
 *
 * Parameters:
 *   color — Input color
 *   factor — Darkness multiplier (0.0-1.0)
 *
 * Returns:
 *   Darkened color
 */
rgb_color_t palette_darken(rgb_color_t color, float factor);

/* ============================================================================
 * PALETTE DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * palette_print_current - Print current palette to console (debug)
 *
 * Displays all 16 colors in the active palette with their RGB values.
 */
void palette_print_current(void);

/**
 * palette_dump_bank - Dump entire palette bank to console (debug)
 *
 * Parameters:
 *   bank — Palette bank to dump
 */
void palette_dump_bank(palette_bank_t bank);

#endif
