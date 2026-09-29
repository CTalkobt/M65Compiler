/* graphics_bitmap_320.c — BITMAP_320x200 Driver Implementation
 *
 * 320×200 monochrome bitmap mode with per-cell color attributes.
 * Phase 2: Full Bresenham line/circle drawing.
 */

#include <graphics.h>
#include <graphics_hal.h>

/* ============================================================================
 * MEMORY LAYOUT
 * ============================================================================
 * Bitmap:    $2000-$3FFF (8KB)  — 320×200 pixels (1 bit per pixel)
 * Screen:    $0400-$07FF (1KB)  — 40×25 cells (attribute/color control)
 * Color:     $D800-$DBFF (1KB)  — Per-cell color (bg<<4 | fg)
 *
 * Pixel addressing:
 *   Cell = (x/8, y/8) → (cell_x, cell_y)
 *   Byte offset = cell_y * 40 + cell_x + (y % 8) * 320
 *   Bit = x % 8
 * ============================================================================ */

#define BITMAP_BASE         0x2000
#define BITMAP_WIDTH        320
#define BITMAP_HEIGHT       200
#define BITMAP_CELL_WIDTH   8

/* ============================================================================
 * DRIVER-LOCAL STATE
 * ============================================================================ */

static unsigned char bitmap_color = 1;       /* Foreground color for cell */
static unsigned char bitmap_bkcolor = 0;     /* Background color for cell */

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * bitmap_get_pixel_byte - Get byte containing pixel (x,y)
 */
static unsigned char *bitmap_get_pixel_byte(int x, int y) {
    unsigned int cell_x = x / BITMAP_CELL_WIDTH;
    unsigned int cell_y = y / BITMAP_CELL_WIDTH;
    unsigned int bit_y = y % BITMAP_CELL_WIDTH;
    unsigned int offset = (unsigned int)BITMAP_BASE + bit_y * 40 + cell_y * 40 * 8 + cell_x;
    return (unsigned char *)offset;
}

/**
 * bitmap_update_cell_color - Update color RAM for (x,y) cell
 */
static void bitmap_update_cell_color(int x, int y) {
    unsigned int cell_x = x / BITMAP_CELL_WIDTH;
    unsigned int cell_y = y / BITMAP_CELL_WIDTH;
    unsigned int cell_offset = cell_y * 40 + cell_x;
    unsigned char *color_ram = GRAPHICS_COLOR_RAM;

    color_ram[cell_offset] = (bitmap_bkcolor << 4) | bitmap_color;
}

/* ============================================================================
 * DRIVER FUNCTIONS
 * ============================================================================ */

static void bitmap_320_init(void) {
    graphics_enable_bitmap_mode();
    graphics_set_bitmap_addr(BITMAP_BASE);
    graphics_set_display_addr(0x0400);
    graphics_set_color_addr(0xD800);

    /* Clear bitmap and color RAM */
    graphics_fill_rect(BITMAP_BASE, 8192, 0x00);          /* Bitmap */
    graphics_fill_rect(0xD800, 1024, 0x11);               /* Colors */
}

static void bitmap_320_done(void) {
    graphics_disable_bitmap_mode();
}

static void bitmap_320_setcolor(unsigned char c) {
    bitmap_color = c & 0x0F;
}

static void bitmap_320_setbkcolor(unsigned char c) {
    bitmap_bkcolor = c & 0x0F;
}

static void bitmap_320_plot(int x, int y) {
    unsigned char *byte_ptr;
    unsigned char bit_x;
    unsigned char mask;

    if (x < 0 || x >= BITMAP_WIDTH || y < 0 || y >= BITMAP_HEIGHT) {
        return;  /* Out of bounds */
    }

    byte_ptr = bitmap_get_pixel_byte(x, y);
    bit_x = x % BITMAP_CELL_WIDTH;
    mask = 0x80 >> bit_x;  /* MSB first */

    *byte_ptr |= mask;  /* Set pixel */
    bitmap_update_cell_color(x, y);
}

static unsigned char bitmap_320_getpixel(int x, int y) {
    unsigned char *byte_ptr;
    unsigned char bit_x;
    unsigned char mask;

    if (x < 0 || x >= BITMAP_WIDTH || y < 0 || y >= BITMAP_HEIGHT) {
        return 0;
    }

    byte_ptr = bitmap_get_pixel_byte(x, y);
    bit_x = x % BITMAP_CELL_WIDTH;
    mask = 0x80 >> bit_x;

    return (*byte_ptr & mask) ? 1 : 0;
}

/**
 * Bresenham line drawing algorithm
 */
static void bitmap_320_line(int x1, int y1, int x2, int y2) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;

    while (1) {
        bitmap_320_plot(x, y);

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
static void bitmap_320_bar(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            bitmap_320_plot(x, y);
        }
    }
}

/**
 * Rectangle outline
 */
static void bitmap_320_rect(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    /* Top and bottom edges */
    for (x = x1; x <= x2; x++) {
        bitmap_320_plot(x, y1);
        bitmap_320_plot(x, y2);
    }

    /* Left and right edges */
    for (y = y1 + 1; y < y2; y++) {
        bitmap_320_plot(x1, y);
        bitmap_320_plot(x2, y);
    }
}

/**
 * Midpoint circle algorithm
 */
static void bitmap_320_circle(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        /* 8-way symmetry */
        bitmap_320_plot(cx + x, cy + y);
        bitmap_320_plot(cx - x, cy + y);
        bitmap_320_plot(cx + x, cy - y);
        bitmap_320_plot(cx - x, cy - y);
        bitmap_320_plot(cx + y, cy + x);
        bitmap_320_plot(cx - y, cy + x);
        bitmap_320_plot(cx + y, cy - x);
        bitmap_320_plot(cx - y, cy - x);

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
static void bitmap_320_clear(void) {
    graphics_fill_rect(BITMAP_BASE, 8192, 0x00);
    graphics_fill_rect(0xD800, 1024, 0x11);
}

/* ============================================================================
 * DRIVER STRUCT
 * ============================================================================ */

graphics_driver_t graphics_driver_bitmap_320x200 = {
    .name = "BITMAP_320x200",
    .mode = GRAPHICS_MODE_BITMAP_320x200,
    .width = 320,
    .height = 200,
    .max_colors = 256,

    .init = bitmap_320_init,
    .done = bitmap_320_done,
    .setcolor = bitmap_320_setcolor,
    .setbkcolor = bitmap_320_setbkcolor,
    .plot = bitmap_320_plot,
    .getpixel = bitmap_320_getpixel,
    .line = bitmap_320_line,
    .bar = bitmap_320_bar,
    .rect = bitmap_320_rect,
    .circle = bitmap_320_circle,
    .clear = bitmap_320_clear
};

