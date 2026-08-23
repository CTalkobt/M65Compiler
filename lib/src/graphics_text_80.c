/* graphics_text_80.c — TEXT_80x24 Driver Implementation
 *
 * 80×24 extended text mode using MEGA65 VIC-IV capabilities.
 * Phase 4: Full implementation with color and drawing support.
 */

#include <graphics.h>
#include <graphics_hal.h>

/* ============================================================================
 * MEMORY LAYOUT (80×24 Extended Mode)
 * ============================================================================
 * Screen:    $2000-$3FFF (8KB)   — 80×24 = 1920 bytes (extended)
 * Color:     $D800-$DBFF + extra — Extended color RAM
 *
 * Note: Extended text mode uses MEGA65's VIC-IV $D031 bit 7
 * Screen RAM moves to $2000-$3FFF to accommodate wider display
 * ============================================================================ */

#define TEXT80_SCREEN_BASE  0x2000
#define TEXT80_WIDTH        80
#define TEXT80_HEIGHT       24
#define TEXT80_COLOR_BASE   0xD800

/* ============================================================================
 * DRIVER-LOCAL STATE
 * ============================================================================ */

static unsigned char text80_color = 1;      /* Foreground color */
static unsigned char text80_bkcolor = 0;    /* Background color */

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * text80_get_screen_offset - Get byte offset for (x,y) in extended screen
 */
static unsigned int text80_get_screen_offset(int x, int y) {
    return y * TEXT80_WIDTH + x;
}

/**
 * text80_get_color_offset - Get color RAM offset for (x,y)
 */
static unsigned int text80_get_color_offset(int x, int y) {
    return y * TEXT80_WIDTH + x;
}

/* ============================================================================
 * DRIVER FUNCTIONS
 * ============================================================================ */

static void text80_init(void) {
    unsigned char *screen_ram;
    unsigned int i;

    /* Enable 80-column extended text mode via VIC-IV $D031 bit 7 */
    graphics_enable_extended_text();

    /* Set display address to $2000 for extended mode */
    graphics_set_display_addr(TEXT80_SCREEN_BASE);
    graphics_set_color_addr(TEXT80_COLOR_BASE);

    /* Clear screen RAM and color RAM */
    screen_ram = (unsigned char *)TEXT80_SCREEN_BASE;
    for (i = 0; i < 1920; i++) {
        screen_ram[i] = ' ';  /* Space character */
    }

    /* Clear color RAM */
    unsigned char *color_ram = (unsigned char *)TEXT80_COLOR_BASE;
    for (i = 0; i < 1920; i++) {
        color_ram[i] = (text80_bkcolor << 4) | text80_color;
    }
}

static void text80_done(void) {
    graphics_disable_extended_text();
}

static void text80_setcolor(unsigned char c) {
    text80_color = c & 0x0F;
}

static void text80_setbkcolor(unsigned char c) {
    text80_bkcolor = c & 0x0F;
}

static void text80_plot(int x, int y) {
    unsigned char *screen_ram;
    unsigned char *color_ram;
    unsigned int offset;

    /* Bounds check */
    if (x < 0 || x >= TEXT80_WIDTH || y < 0 || y >= TEXT80_HEIGHT) {
        return;
    }

    screen_ram = (unsigned char *)TEXT80_SCREEN_BASE;
    color_ram = (unsigned char *)TEXT80_COLOR_BASE;
    offset = text80_get_screen_offset(x, y);

    /* Plot space character with current colors */
    screen_ram[offset] = ' ';
    color_ram[offset] = (text80_bkcolor << 4) | text80_color;
}

static unsigned char text80_getpixel(int x, int y) {
    unsigned char *screen_ram;
    unsigned int offset;

    if (x < 0 || x >= TEXT80_WIDTH || y < 0 || y >= TEXT80_HEIGHT) {
        return 0;
    }

    screen_ram = (unsigned char *)TEXT80_SCREEN_BASE;
    offset = text80_get_screen_offset(x, y);
    return screen_ram[offset];
}

/**
 * Draw line using simple Bresenham-style algorithm in text mode
 */
static void text80_line(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps;
    int i;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    steps = (dx > dy) ? dx : dy;

    if (steps > 0) {
        for (i = 0; i <= steps; i++) {
            int x = x1 + (i * (x2 - x1)) / steps;
            int y = y1 + (i * (y2 - y1)) / steps;
            text80_plot(x, y);
        }
    }
}

/**
 * Draw filled rectangle in text mode
 */
static void text80_bar(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            text80_plot(x, y);
        }
    }
}

/**
 * Draw rectangle outline in text mode
 */
static void text80_rect(int x1, int y1, int x2, int y2) {
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    /* Top and bottom edges */
    for (x = x1; x <= x2; x++) {
        text80_plot(x, y1);
        text80_plot(x, y2);
    }

    /* Left and right edges */
    for (y = y1 + 1; y < y2; y++) {
        text80_plot(x1, y);
        text80_plot(x2, y);
    }
}

/**
 * Draw circle in text mode (approximate)
 */
static void text80_circle(int cx, int cy, int r) {
    int x, y;
    int i;

    for (i = 0; i < 360; i += 45) {
        /* Simple angle approximation */
        int sine_val = 0, cos_val = r;

        if (i >= 45 && i < 135) {
            sine_val = r;
        } else if (i >= 135 && i < 225) {
            sine_val = r;
            cos_val = -r;
        } else if (i >= 225 && i < 315) {
            cos_val = -r;
        }

        x = cx + cos_val;
        y = cy + sine_val;
        text80_plot(x, y);
    }
}

/**
 * Clear entire screen in 80-column mode
 */
static void text80_clear(void) {
    unsigned char *screen_ram;
    unsigned int i;

    screen_ram = (unsigned char *)TEXT80_SCREEN_BASE;
    for (i = 0; i < 1920; i++) {
        screen_ram[i] = ' ';
    }
}

/* ============================================================================
 * DRIVER STRUCT
 * ============================================================================ */

graphics_driver_t graphics_driver_text_80x24 = {
    .name = "TEXT_80x24",
    .mode = GRAPHICS_MODE_TEXT_80x24,
    .width = 80,
    .height = 24,
    .max_colors = 16,

    .init = text80_init,
    .done = text80_done,
    .setcolor = text80_setcolor,
    .setbkcolor = text80_setbkcolor,
    .plot = text80_plot,
    .getpixel = text80_getpixel,
    .line = text80_line,
    .bar = text80_bar,
    .rect = text80_rect,
    .circle = text80_circle,
    .clear = text80_clear
};

