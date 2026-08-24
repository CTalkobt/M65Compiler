/* graphics_graphics_80.c — GRAPHICS_80x50 Driver Implementation
 *
 * 80×50 graphics mode using block-drawing characters for pixel simulation.
 * Phase 6: Virtual pixel grid with PETSCII block characters.
 */

#include <graphics.h>
#include <graphics_hal.h>

/* ============================================================================
 * MEMORY LAYOUT (80×50 Graphics Mode)
 * ============================================================================
 * Virtual Grid:  80×50 (4000 pixels)
 * Display:       80×25 characters (each char = 2 vertical pixels)
 * Screen RAM:    $2000-$3FFF (8KB for 80×25)
 * Color RAM:     Extended for 80×25
 * Framebuffer:   $4000-$4F9F (4000 bytes for 80×50 virtual grid)
 *
 * Mapping:
 *   Virtual pixel (x, y) → Character at (x, y/2)
 *   Top pixel (y%2==0):    Use TOP_BLOCK character (▀)
 *   Bottom pixel (y%2==1): Use BOTTOM_BLOCK character (▄)
 * ============================================================================ */

#define GRAPHICS80_WIDTH            80
#define GRAPHICS80_HEIGHT           50
#define GRAPHICS80_DISPLAY_HEIGHT   25  /* 50 / 2 */
#define GRAPHICS80_FRAMEBUFFER_BASE 0x4000
#define GRAPHICS80_SCREEN_BASE      0x2000

/* PETSCII block characters */
#define CHAR_TOP_BLOCK    0x61      /* ▀ (97 decimal) */
#define CHAR_BOTTOM_BLOCK 0x62      /* ▄ (98 decimal) */
#define CHAR_FULL_BLOCK   0x60      /* █ (96 decimal) */
#define CHAR_SPACE        0x20      /* Space (32) */

/* ============================================================================
 * DRIVER-LOCAL STATE
 * ============================================================================ */

static unsigned char graphics80_color = 1;
static unsigned char graphics80_bkcolor = 0;
static unsigned char *framebuffer = (unsigned char *)GRAPHICS80_FRAMEBUFFER_BASE;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * graphics80_plot_virtual - Set virtual pixel (x,y) in framebuffer
 */
static void graphics80_plot_virtual(int x, int y, unsigned char color) {
    if (x < 0 || x >= GRAPHICS80_WIDTH || y < 0 || y >= GRAPHICS80_HEIGHT) {
        return;
    }

    unsigned int offset = y * GRAPHICS80_WIDTH + x;
    framebuffer[offset] = color;
}

/**
 * graphics80_get_virtual - Get virtual pixel color
 */
static unsigned char graphics80_get_virtual(int x, int y) {
    if (x < 0 || x >= GRAPHICS80_WIDTH || y < 0 || y >= GRAPHICS80_HEIGHT) {
        return 0;
    }

    unsigned int offset = y * GRAPHICS80_WIDTH + x;
    return framebuffer[offset];
}

/**
 * graphics80_update_display - Sync framebuffer to screen RAM
 * Convert 80×50 virtual grid to 80×25 display using block characters
 */
static void graphics80_update_display(void) {
    unsigned char *screen_ram = (unsigned char *)GRAPHICS80_SCREEN_BASE;
    unsigned char *color_ram = GRAPHICS80_SCREEN_BASE + 0xD800 - 0x2000;
    int x, y;

    for (y = 0; y < GRAPHICS80_DISPLAY_HEIGHT; y++) {
        for (x = 0; x < GRAPHICS80_WIDTH; x++) {
            unsigned char top_color = graphics80_get_virtual(x, y * 2);
            unsigned char bot_color = graphics80_get_virtual(x, y * 2 + 1);

            /* Determine character to use */
            unsigned char char_val;
            if (top_color > 0 && bot_color > 0) {
                char_val = CHAR_FULL_BLOCK;
            } else if (top_color > 0) {
                char_val = CHAR_TOP_BLOCK;
            } else if (bot_color > 0) {
                char_val = CHAR_BOTTOM_BLOCK;
            } else {
                char_val = CHAR_SPACE;
            }

            unsigned int offset = y * GRAPHICS80_WIDTH + x;
            screen_ram[offset] = char_val;
            color_ram[offset] = (bot_color << 4) | top_color;
        }
    }
}

/* ============================================================================
 * DRIVER FUNCTIONS
 * ============================================================================ */

static void graphics80_init(void) {
    /* Enable 80×24 extended text mode */
    graphics_enable_extended_text();
    graphics_set_display_addr(GRAPHICS80_SCREEN_BASE);

    /* Clear framebuffer */
    unsigned int i;
    for (i = 0; i < GRAPHICS80_WIDTH * GRAPHICS80_HEIGHT; i++) {
        framebuffer[i] = 0;
    }

    /* Initial display update */
    graphics80_update_display();
}

static void graphics80_done(void) {
    graphics_disable_extended_text();
}

static void graphics80_setcolor(unsigned char c) {
    graphics80_color = c & 0x0F;
}

static void graphics80_setbkcolor(unsigned char c) {
    graphics80_bkcolor = c & 0x0F;
}

static void graphics80_plot(int x, int y) {
    graphics80_plot_virtual(x, y, graphics80_color);
    graphics80_update_display();
}

static unsigned char graphics80_getpixel(int x, int y) {
    return graphics80_get_virtual(x, y);
}

/**
 * Bresenham line drawing
 */
static void graphics80_line(int x1, int y1, int x2, int y2) {
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;

    while (1) {
        graphics80_plot_virtual(x, y, graphics80_color);

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

    graphics80_update_display();
}

/**
 * Filled rectangle
 */
static void graphics80_bar(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            graphics80_plot_virtual(x, y, graphics80_color);
        }
    }

    graphics80_update_display();
}

/**
 * Rectangle outline
 */
static void graphics80_rect(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    /* Top and bottom */
    for (x = x1; x <= x2; x++) {
        graphics80_plot_virtual(x, y1, graphics80_color);
        graphics80_plot_virtual(x, y2, graphics80_color);
    }

    /* Left and right */
    for (y = y1 + 1; y < y2; y++) {
        graphics80_plot_virtual(x1, y, graphics80_color);
        graphics80_plot_virtual(x2, y, graphics80_color);
    }

    graphics80_update_display();
}

/**
 * Midpoint circle algorithm
 */
static void graphics80_circle(int cx, int cy, int r) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        /* 8-way symmetry */
        graphics80_plot_virtual(cx + x, cy + y, graphics80_color);
        graphics80_plot_virtual(cx - x, cy + y, graphics80_color);
        graphics80_plot_virtual(cx + x, cy - y, graphics80_color);
        graphics80_plot_virtual(cx - x, cy - y, graphics80_color);
        graphics80_plot_virtual(cx + y, cy + x, graphics80_color);
        graphics80_plot_virtual(cx - y, cy + x, graphics80_color);
        graphics80_plot_virtual(cx + y, cy - x, graphics80_color);
        graphics80_plot_virtual(cx - y, cy - x, graphics80_color);

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    graphics80_update_display();
}

/**
 * Clear entire graphics display
 */
static void graphics80_clear(void) {
    unsigned int i;

    for (i = 0; i < GRAPHICS80_WIDTH * GRAPHICS80_HEIGHT; i++) {
        framebuffer[i] = 0;
    }

    graphics80_update_display();
}

/* ============================================================================
 * DRIVER STRUCT
 * ============================================================================ */

graphics_driver_t graphics_driver_graphics_80x50 = {
    .name = "GRAPHICS_80x50",
    .mode = GRAPHICS_MODE_GRAPHICS_80x50,
    .width = 80,
    .height = 50,
    .max_colors = 256,

    .init = graphics80_init,
    .done = graphics80_done,
    .setcolor = graphics80_setcolor,
    .setbkcolor = graphics80_setbkcolor,
    .plot = graphics80_plot,
    .getpixel = graphics80_getpixel,
    .line = graphics80_line,
    .bar = graphics80_bar,
    .rect = graphics80_rect,
    .circle = graphics80_circle,
    .clear = graphics80_clear
};

