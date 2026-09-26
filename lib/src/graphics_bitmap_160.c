/* graphics_bitmap_160.c — BITMAP_160x200 Driver Implementation
 *
 * 160×200 multicolor bitmap mode with 4 colors per 8×8 cell.
 * Phase 5: Full color palette and drawing primitives.
 */

#include <graphics.h>
#include <graphics_hal.h>

/* ============================================================================
 * MEMORY LAYOUT
 * ============================================================================
 * Same bitmap as 320×200 but interpreted as 2 bits per pixel:
 *   Bitmap:    $2000-$3FFF (8KB)  — 160×200 pixels (2 bits per pixel)
 *   Screen:    $0400-$07FF (1KB)  — 40×25 cells (palette selection)
 *   Color:     $D800-$DBFF (1KB)  — Per-cell palette selection
 *   Palette:   External array (16 colors × 4 colors per cell)
 *
 * Pixel addressing (2 bits per pixel):
 *   Cell = (x/8, y/8)
 *   Byte offset = cell_y * 40 + cell_x + (y % 8) * 200  (note: 200 not 320)
 *   Bits = (x % 4) * 2  (4 pixels per byte)
 * ============================================================================ */

#define BITMAP_BASE         0x2000
#define BITMAP_WIDTH        160
#define BITMAP_HEIGHT       200
#define BITMAP_CELL_WIDTH   8

/* Color palette: 16 cells × 4 colors each = 64 bytes total */
static unsigned char palette[16][4] = {
    {0, 1, 2, 3},     /* Cell 0: Black, White, Red, Cyan */
    {0, 5, 6, 4},     /* Cell 1: Black, Green, Blue, Magenta */
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4},
    {0, 1, 2, 3},
    {0, 5, 6, 4}
};

/* ============================================================================
 * DRIVER-LOCAL STATE
 * ============================================================================ */

static unsigned char multicolor_color = 1;
static unsigned char multicolor_bkcolor = 0;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * multicolor_get_pixel_byte - Get byte containing pixel (x,y)
 */
static unsigned char *multicolor_get_pixel_byte(int x, int y) {
    unsigned int cell_y = y / BITMAP_CELL_WIDTH;
    unsigned int bit_y = y % BITMAP_CELL_WIDTH;
    unsigned int offset = (unsigned int)BITMAP_BASE + bit_y * 200 + cell_y * 40 + (x / 4);
    return (unsigned char *)offset;
}

/**
 * multicolor_set_pixel_color - Set color index for pixel (x,y)
 * Color index 0-3, maps to palette via screen RAM
 */
static void multicolor_set_pixel_color(int x, int y, unsigned char color) {
    unsigned char *byte_ptr;
    unsigned int cell_x = x / BITMAP_CELL_WIDTH;
    unsigned int cell_y = y / BITMAP_CELL_WIDTH;
    unsigned int bit_x = (x % BITMAP_CELL_WIDTH) / 2;  /* 4 pixels per byte */
    unsigned char shift = (3 - bit_x) * 2;  /* MSB first */
    unsigned char mask = 0x03 << shift;

    byte_ptr = multicolor_get_pixel_byte(x, y);

    /* Clear the 2 bits for this pixel, then set new color */
    *byte_ptr = (*byte_ptr & ~mask) | ((color & 0x03) << shift);
}

/**
 * multicolor_get_pixel_color - Read 2-bit color index for pixel
 */
static unsigned char multicolor_get_pixel_color(int x, int y) {
    unsigned char *byte_ptr;
    unsigned int bit_x = (x % BITMAP_CELL_WIDTH) / 2;
    unsigned char shift = (3 - bit_x) * 2;

    byte_ptr = multicolor_get_pixel_byte(x, y);
    return (*byte_ptr >> shift) & 0x03;
}

/**
 * multicolor_get_cell_palette_index - Get palette index for cell (x,y)
 */
static unsigned int multicolor_get_cell_palette(int x, int y) {
    unsigned int cell_x = x / BITMAP_CELL_WIDTH;
    unsigned int cell_y = y / BITMAP_CELL_WIDTH;
    unsigned int cell_offset = cell_y * 40 + cell_x;
    unsigned char *screen_ram = GRAPHICS_SCREEN_RAM;

    return (screen_ram[cell_offset] >> 4) & 0x0F;  /* Upper nibble = palette */
}

/* ============================================================================
 * DRIVER FUNCTIONS
 * ============================================================================ */

static void multicolor_init(void) {
    graphics_enable_bitmap_mode();
    graphics_enable_multicolor();
    graphics_set_bitmap_addr(BITMAP_BASE);
    graphics_set_display_addr(0x0400);
    graphics_set_color_addr(0xD800);

    /* Clear bitmap and screen RAM */
    graphics_fill_rect(BITMAP_BASE, 8192, 0x00);
    graphics_fill_rect(0x0400, 1024, 0x00);
    graphics_fill_rect(0xD800, 1024, 0x11);
}

static void multicolor_done(void) {
    graphics_disable_multicolor();
    graphics_disable_bitmap_mode();
}

static void multicolor_setcolor(unsigned char c) {
    multicolor_color = c & 0x0F;
}

static void multicolor_setbkcolor(unsigned char c) {
    multicolor_bkcolor = c & 0x0F;
}

static void multicolor_plot(int x, int y) {
    unsigned int palette_idx;

    if (x < 0 || x >= BITMAP_WIDTH || y < 0 || y >= BITMAP_HEIGHT) {
        return;  /* Out of bounds */
    }

    /* Get palette for this cell */
    palette_idx = multicolor_get_cell_palette(x, y);

    /* Use foreground color from palette */
    unsigned char color_idx = 1;  /* Default to second color in palette */
    if (multicolor_color < 4) {
        color_idx = multicolor_color;
    }

    multicolor_set_pixel_color(x, y, color_idx);
}

static unsigned char multicolor_getpixel(int x, int y) {
    unsigned int palette_idx;
    unsigned char color_idx;

    if (x < 0 || x >= BITMAP_WIDTH || y < 0 || y >= BITMAP_HEIGHT) {
        return 0;
    }

    palette_idx = multicolor_get_cell_palette(x, y);
    color_idx = multicolor_get_pixel_color(x, y);

    /* Map color index to actual color via palette */
    return palette[palette_idx][color_idx];
}

/**
 * Bresenham line drawing for multicolor mode
 */
static void multicolor_line(int x1, int y1, int x2, int y2) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;

    while (1) {
        multicolor_plot(x, y);

        if (x == x2 && y == y2) break;

        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

/**
 * Filled rectangle (scan line fill)
 */
static void multicolor_bar(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            multicolor_plot(x, y);
        }
    }
}

/**
 * Rectangle outline
 */
static void multicolor_rect(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    /* Top and bottom edges */
    for (x = x1; x <= x2; x++) {
        multicolor_plot(x, y1);
        multicolor_plot(x, y2);
    }

    /* Left and right edges */
    for (y = y1 + 1; y < y2; y++) {
        multicolor_plot(x1, y);
        multicolor_plot(x2, y);
    }
}

/**
 * Midpoint circle algorithm for multicolor mode
 */
static void multicolor_circle(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        /* 8-way symmetry */
        multicolor_plot(cx + x, cy + y);
        multicolor_plot(cx - x, cy + y);
        multicolor_plot(cx + x, cy - y);
        multicolor_plot(cx - x, cy - y);
        multicolor_plot(cx + y, cy + x);
        multicolor_plot(cx - y, cy + x);
        multicolor_plot(cx + y, cy - x);
        multicolor_plot(cx - y, cy - x);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

/**
 * Clear entire bitmap
 */
static void multicolor_clear(void) {
    graphics_fill_rect(BITMAP_BASE, 8192, 0x00);
    graphics_fill_rect(0x0400, 1024, 0x00);
}

/* ============================================================================
 * DRIVER STRUCT
 * ============================================================================ */

graphics_driver_t graphics_driver_bitmap_160x200 = {
    .name = "BITMAP_160x200",
    .mode = GRAPHICS_MODE_BITMAP_160x200,
    .width = 160,
    .height = 200,
    .max_colors = 256,  /* Via palette lookup */

    .init = multicolor_init,
    .done = multicolor_done,
    .setcolor = multicolor_setcolor,
    .setbkcolor = multicolor_setbkcolor,
    .plot = multicolor_plot,
    .getpixel = multicolor_getpixel,
    .line = multicolor_line,
    .bar = multicolor_bar,
    .rect = multicolor_rect,
    .circle = multicolor_circle,
    .clear = multicolor_clear
};

