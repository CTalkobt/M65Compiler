/* graphics_rrb.h — Raster Re-Write Buffer (Phase 105)
 *
 * Per-raster-line character repositioning using GOTOX instructions.
 * Object-oriented API with struct methods (auto-inlined, zero overhead).
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
 * RRB Layer definition (with methods)
 *
 * Methods are all trivial (≤3 statements) and auto-inlined.
 * No performance overhead vs. C-style functions.
 */
typedef struct rrb_layer {
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

    /* Method pointers */

    void (*show)(struct rrb_layer *this);
    void (*hide)(struct rrb_layer *this);
    void (*set_scroll)(struct rrb_layer *this, int sx, int sy);
    void (*set_priority)(struct rrb_layer *this, int p);
    void (*set_position)(struct rrb_layer *this, int x, int y);
    int (*is_visible)(struct rrb_layer *this);
    int (*get_priority)(struct rrb_layer *this);
    int (*get_scroll_x)(struct rrb_layer *this);
    int (*get_scroll_y)(struct rrb_layer *this);
    void (*set_char)(struct rrb_layer *this, int col, int row, unsigned char ch, unsigned char color);
    unsigned char (*get_char)(struct rrb_layer *this, int col, int row);
    unsigned char (*get_color)(struct rrb_layer *this, int col, int row);
    void (*clear)(struct rrb_layer *this, unsigned char fill_char, unsigned char fill_color);
    unsigned char *(*screen_ptr)(struct rrb_layer *this);
    unsigned char *(*color_ptr)(struct rrb_layer *this);

} rrb_layer_t;

/**
 * RRB System state (with methods)
 */
typedef struct rrb_system {
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

    /* Method pointers */

    int (*is_enabled)(struct rrb_system *this);
    int (*get_layer_count)(struct rrb_system *this);
    int (*get_screen_width)(struct rrb_system *this);
    int (*get_screen_height)(struct rrb_system *this);
    void (*enable)(struct rrb_system *this);
    void (*disable)(struct rrb_system *this);
    int (*is_double_time)(struct rrb_system *this);

    int (*init)(struct rrb_system *this, int max_layers, int width, int height);
    void (*done)(struct rrb_system *this);

    int (*create_layer)(struct rrb_system *this, rrb_layer_mode_t mode, int width, int height);
    void (*destroy_layer)(struct rrb_system *this, int layer_idx);

    rrb_layer_t *(*get_layer)(struct rrb_system *this, int layer_idx);

    int (*configure_vic)(struct rrb_system *this, int h640, int chrcount, int linestep);
    int (*enable_double_time)(struct rrb_system *this);
    void (*disable_double_time)(struct rrb_system *this);

    int (*render)(struct rrb_system *this);
    int (*render_row)(struct rrb_system *this, int row);
    void (*update)(struct rrb_system *this);
    void (*sync_display)(struct rrb_system *this);

} rrb_system_t;

/* ============================================================================
 * GOTOX UTILITIES (Low-level, not methods)
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
 * Returns:
 *   Required CHRCOUNT value
 */
int rrb_calc_chrcount(int num_chars, int num_gotox);

/**
 * rrb_calc_linestep - Calculate required LINESTEP
 *
 * Returns:
 *   Required LINESTEP value
 */
int rrb_calc_linestep(int chrcount);

/**
 * rrb_test_raster_budget - Check if scene fits in raster time
 *
 * Returns:
 *   1 if fits, 0 if exceeds budget
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

