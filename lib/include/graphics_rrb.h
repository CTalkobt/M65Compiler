/* graphics_rrb.h — Raster Re-Write Buffer (Phase 105)
 *
 * Per-raster-line character repositioning using GOTOX instructions.
 * Enables unlimited soft sprites, parallax scrolling, and multi-layer composition.
 *
 * Architecture: RRB works with TEXT mode to layer characters using GOTOX tokens.
 * Each raster line can have multiple GOTOX repositioning commands to draw layers
 * at arbitrary pixel positions.
 */

#pragma once

/* ============================================================================
 * RRB LAYER STRUCTURES
 * ============================================================================ */

/**
 * RRB Layer rendering mode
 */
typedef enum {
    RRB_MODE_FULL = 0,      /* Full-width layer (entire screen) */
    RRB_MODE_SPARSE = 1,    /* Sparse objects with transparent spaces */
    RRB_MODE_STACK = 2      /* Interleaved stack at each position */
} rrb_layer_mode_t;

/**
 * RRB Layer definition
 */
typedef struct {
    unsigned int screen_addr;       /* Base screen memory address */
    unsigned int color_addr;        /* Base color memory address */
    int width;                      /* Width in characters */
    int height;                     /* Height in rows */
    int scroll_x;                   /* Horizontal scroll (pixels) */
    int scroll_y;                   /* Vertical scroll (pixels) */
    rrb_layer_mode_t mode;          /* Rendering mode */
    int transparent_char;           /* Character to treat as transparent (sparse mode) */
    int visible;                    /* 1 = visible, 0 = hidden */
    int priority;                   /* Layer order (0=bottom, higher=on top) */
} rrb_layer_t;

/**
 * GOTOX instruction (internal use)
 *
 * Encodes horizontal repositioning command for VIC-IV.
 */
typedef struct {
    int pixel_x;                    /* 10-bit pixel position (0-1023) */
    int transparent;                /* 1 = transparent ($00 pixels) */
} rrb_gotox_t;

/**
 * RRB System state
 */
typedef struct {
    rrb_layer_t *layers;            /* Array of layers */
    int layer_count;                /* Number of layers */
    int max_layers;                 /* Max layers allocated */
    int enabled;                    /* 1 = RRB active, 0 = disabled */

    /* VIC-IV Configuration */
    int screen_width;               /* Screen width in pixels (320 or 640) */
    int screen_height;              /* Screen height in rows (25 or 50) */
    int chrcount;                   /* CHRCOUNT register (chars per row) */
    int linestep;                   /* LINESTEP register (bytes per row) */
    int h640;                       /* 1 = 640px mode, 0 = 320px mode */

    /* Double-Raster-Time Mode (DBLRR) */
    int double_raster_time;         /* 1 = enabled, 0 = normal */

    /* Working buffers */
    unsigned char *screen_buffer;   /* Composite screen memory */
    unsigned char *color_buffer;    /* Composite color memory */
    int buffer_size;
} rrb_system_t;

/* ============================================================================
 * INITIALIZATION & LIFECYCLE
 * ============================================================================ */

/**
 * rrb_init - Initialize RRB system
 *
 * Must be called in TEXT mode (TEXT_40x25 or TEXT_80x24).
 *
 * Parameters:
 *   rrb — RRB system (caller-allocated)
 *   max_layers — Maximum layers to support (typical: 2-4)
 *   width — Screen width in characters (40 or 80)
 *   height — Screen height in rows (25, 24, or 50)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int rrb_init(rrb_system_t *rrb, int max_layers, int width, int height);

/**
 * rrb_done - Cleanup RRB system
 *
 * Frees allocated memory and restores VIC-IV.
 */
void rrb_done(rrb_system_t *rrb);

/* ============================================================================
 * RRB ENABLE/DISABLE
 * ============================================================================ */

/**
 * rrb_enable - Activate RRB rendering
 *
 * Enables RRB mode and starts layer composition.
 */
void rrb_enable(rrb_system_t *rrb);

/**
 * rrb_disable - Deactivate RRB rendering
 *
 * Stops RRB and restores normal text mode.
 */
void rrb_disable(rrb_system_t *rrb);

/**
 * rrb_is_enabled - Check RRB status
 *
 * Returns:
 *   1 if RRB enabled, 0 if disabled
 */
int rrb_is_enabled(rrb_system_t *rrb);

/* ============================================================================
 * LAYER MANAGEMENT
 * ============================================================================ */

/**
 * rrb_layer_create - Add layer to RRB system
 *
 * Allocates screen and color memory for layer.
 *
 * Parameters:
 *   rrb — RRB system
 *   mode — Layer rendering mode (RRB_MODE_FULL, RRB_MODE_SPARSE, etc.)
 *   width — Layer width in characters
 *   height — Layer height in rows
 *
 * Returns:
 *   Layer index (0+) on success, -1 on error
 */
int rrb_layer_create(rrb_system_t *rrb, rrb_layer_mode_t mode, int width, int height);

/**
 * rrb_layer_destroy - Remove layer
 *
 * Parameters:
 *   rrb — RRB system
 *   layer_idx — Layer index
 */
void rrb_layer_destroy(rrb_system_t *rrb, int layer_idx);

/**
 * rrb_get_layer - Get layer by index
 *
 * Parameters:
 *   rrb — RRB system
 *   layer_idx — Layer index (0 = bottom)
 *
 * Returns:
 *   Layer pointer, or NULL if invalid
 */
rrb_layer_t *rrb_get_layer(rrb_system_t *rrb, int layer_idx);

/* ============================================================================
 * LAYER CONFIGURATION
 * ============================================================================ */

/**
 * rrb_layer_set_scroll - Set layer scroll position
 *
 * Parameters:
 *   layer — Layer to configure
 *   scroll_x, scroll_y — Scroll offset (pixels)
 */
void rrb_layer_set_scroll(rrb_layer_t *layer, int scroll_x, int scroll_y);

/**
 * rrb_layer_set_position - Set layer screen position
 *
 * For sparse/stack modes: starting pixel position on screen.
 *
 * Parameters:
 *   layer — Layer to configure
 *   x, y — Position (pixels)
 */
void rrb_layer_set_position(rrb_layer_t *layer, int x, int y);

/**
 * rrb_layer_show - Make layer visible
 *
 * Parameters:
 *   layer — Layer to show
 */
void rrb_layer_show(rrb_layer_t *layer);

/**
 * rrb_layer_hide - Hide layer
 *
 * Parameters:
 *   layer — Layer to hide
 */
void rrb_layer_hide(rrb_layer_t *layer);

/**
 * rrb_layer_set_priority - Reorder layer (z-order)
 *
 * Higher priority = rendered on top.
 *
 * Parameters:
 *   layer — Layer to reorder
 *   priority — Priority level (0 = bottom)
 */
void rrb_layer_set_priority(rrb_layer_t *layer, int priority);

/* ============================================================================
 * LAYER DATA ACCESS
 * ============================================================================ */

/**
 * rrb_layer_set_char - Set character at position
 *
 * Parameters:
 *   layer — Layer
 *   col, row — Character position
 *   ch — Character code
 *   color — Color byte (with attribute bits)
 */
void rrb_layer_set_char(rrb_layer_t *layer, int col, int row,
                        unsigned char ch, unsigned char color);

/**
 * rrb_layer_get_char - Get character at position
 *
 * Returns:
 *   Character code
 */
unsigned char rrb_layer_get_char(rrb_layer_t *layer, int col, int row);

/**
 * rrb_layer_get_color - Get color byte at position
 *
 * Returns:
 *   Color byte (with attribute bits)
 */
unsigned char rrb_layer_get_color(rrb_layer_t *layer, int col, int row);

/**
 * rrb_layer_clear - Fill layer with character
 *
 * Parameters:
 *   layer — Layer to clear
 *   fill_char — Character to fill with (typically space=32)
 *   fill_color — Color to fill with
 */
void rrb_layer_clear(rrb_layer_t *layer, unsigned char fill_char, unsigned char fill_color);

/**
 * rrb_layer_screen_ptr - Get screen memory pointer
 *
 * For direct manipulation (fast bulk operations).
 *
 * Returns:
 *   Pointer to screen memory for layer
 */
unsigned char *rrb_layer_screen_ptr(rrb_layer_t *layer);

/**
 * rrb_layer_color_ptr - Get color memory pointer
 *
 * Returns:
 *   Pointer to color memory for layer
 */
unsigned char *rrb_layer_color_ptr(rrb_layer_t *layer);

/* ============================================================================
 * RENDERING
 * ============================================================================ */

/**
 * rrb_render - Build composite RRB screen from all layers
 *
 * Injects GOTOX instructions between layers for repositioning.
 * Must be called after layer modifications, before display.
 *
 * Parameters:
 *   rrb — RRB system
 *
 * Returns:
 *   0 on success, -1 if raster time exceeded
 */
int rrb_render(rrb_system_t *rrb);

/**
 * rrb_render_row - Build single row with RRB
 *
 * For row-by-row updates or partial rendering.
 *
 * Parameters:
 *   rrb — RRB system
 *   row — Row to render (0-24 or 0-49)
 *
 * Returns:
 *   0 on success, -1 if raster time exceeded
 */
int rrb_render_row(rrb_system_t *rrb, int row);

/**
 * rrb_update - Update and render all layers
 *
 * Calls rrb_render() and applies scroll positions.
 *
 * Parameters:
 *   rrb — RRB system
 */
void rrb_update(rrb_system_t *rrb);

/**
 * rrb_sync_display - Wait for raster and apply RRB to screen
 *
 * Atomic update: copy composite buffer to VIC-IV memory.
 * Safe to call every frame.
 *
 * Parameters:
 *   rrb — RRB system
 */
void rrb_sync_display(rrb_system_t *rrb);

/* ============================================================================
 * VIC-IV CONFIGURATION
 * ============================================================================ */

/**
 * rrb_configure_vic - Configure VIC-IV for RRB
 *
 * Sets CHRCOUNT, LINESTEP, and other registers.
 *
 * Parameters:
 *   rrb — RRB system
 *   h640 — 1 = 640px mode, 0 = 320px mode
 *   chrcount — Characters per row (10-bit, 0-1023)
 *   linestep — Bytes per row in screen memory (16-bit)
 *
 * Returns:
 *   0 on success, -1 if invalid parameters
 */
int rrb_configure_vic(rrb_system_t *rrb, int h640, int chrcount, int linestep);

/**
 * rrb_enable_double_time - Enable double-raster-time mode (DBLRR)
 *
 * Doubles available raster time but adjusts vertical resolution.
 * Use if scenes exceed single-raster budget.
 *
 * Parameters:
 *   rrb — RRB system
 *
 * Returns:
 *   0 on success
 */
int rrb_enable_double_time(rrb_system_t *rrb);

/**
 * rrb_disable_double_time - Disable double-raster-time mode
 *
 * Parameters:
 *   rrb — RRB system
 */
void rrb_disable_double_time(rrb_system_t *rrb);

/* ============================================================================
 * GOTOX UTILITIES (Low-level)
 * ============================================================================ */

/**
 * rrb_write_gotox - Write GOTOX instruction to memory
 *
 * Low-level function for manual GOTOX injection.
 *
 * Parameters:
 *   screen_addr — Screen memory address
 *   color_addr — Color memory address
 *   pixel_x — 10-bit pixel position (0-1023)
 *   transparent — 1 = enable transparency, 0 = normal
 */
void rrb_write_gotox(unsigned int screen_addr, unsigned int color_addr,
                     int pixel_x, int transparent);

/**
 * rrb_calc_chrcount - Calculate required CHRCOUNT
 *
 * CHRCOUNT must accommodate all characters and GOTOX instructions per row.
 *
 * Returns:
 *   Required CHRCOUNT value
 */
int rrb_calc_chrcount(int num_chars, int num_gotox);

/**
 * rrb_calc_linestep - Calculate required LINESTEP
 *
 * LINESTEP must be >= CHRCOUNT for single-byte encoding.
 *
 * Returns:
 *   Required LINESTEP value
 */
int rrb_calc_linestep(int chrcount);

/**
 * rrb_test_raster_budget - Check if scene fits in raster time
 *
 * Estimates if GOTOX instructions + characters exceed raster budget.
 *
 * Parameters:
 *   rrb — RRB system
 *   row — Row to test
 *
 * Returns:
 *   1 if fits, 0 if exceeds budget (use DBLRR)
 */
int rrb_test_raster_budget(rrb_system_t *rrb, int row);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define RRB_MAX_LAYERS          10      /* Maximum layers per system */
#define RRB_MAX_CHRCOUNT        1023    /* Max characters per row (10-bit) */
#define RRB_SCREEN_WIDTH_320    320     /* 320px mode width */
#define RRB_SCREEN_WIDTH_640    640     /* 640px mode width */

/* GOTOX encoding bits */
#define RRB_GOTOX_FLAG          0x10    /* Color byte bit 4: GOTOX flag */
#define RRB_TRANSPARENCY_FLAG    0x80    /* Color byte bit 7: transparency */

