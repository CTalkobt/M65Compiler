/* graphics_text_40.c — TEXT_40x25 Driver Implementation
 *
 * Standard 40×25 text mode driver using conio functions.
 */

#include <graphics.h>
#include <conio.h>

/* ============================================================================
 * DRIVER-LOCAL STATE
 * ============================================================================ */

static unsigned char text_color = 1;
static unsigned char text_bkcolor = 0;

/* ============================================================================
 * DRIVER FUNCTIONS
 * ============================================================================ */

static void text_40_init(void) {
    clrscr();
    textcolor(text_color);
    textbackground(text_bkcolor);
}

static void text_40_done(void) {
    /* Restore to white on black */
    textcolor(1);
    textbackground(0);
}

static void text_40_setcolor(unsigned char c) {
    text_color = c & 0x0F;
    textcolor(text_color);
}

static void text_40_setbkcolor(unsigned char c) {
    text_bkcolor = c & 0x0F;
    textbackground(text_bkcolor);
}

static void text_40_plot(int x, int y) {
    /* Text mode: plot a space character at position */
    if (x >= 0 && x < 40 && y >= 0 && y < 25) {
        cputch(x, y, ' ');
    }
}

static unsigned char text_40_getpixel(int x, int y) {
    /* Text mode: return character at position */
    if (x >= 0 && x < 40 && y >= 0 && y < 25) {
        return getch_xy(x, y);
    }
    return 0;
}

static void text_40_line(int x1, int y1, int x2, int y2) {
    /* Simple line drawing in text mode */
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps;
    int i;

    /* Use octant with most steps */
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    steps = (dx > dy) ? dx : dy;

    if (steps > 0) {
        for (i = 0; i <= steps; i++) {
            int x = x1 + (i * (x2 - x1)) / steps;
            int y = y1 + (i * (y2 - y1)) / steps;
            text_40_plot(x, y);
        }
    }
}

static void text_40_bar(int x1, int y1, int x2, int y2) {
    /* Filled rectangle in text mode */
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            text_40_plot(x, y);
        }
    }
}

static void text_40_rect(int x1, int y1, int x2, int y2) {
    /* Rectangle outline in text mode */
    int x, y;

    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    /* Top and bottom */
    for (x = x1; x <= x2; x++) {
        text_40_plot(x, y1);
        text_40_plot(x, y2);
    }

    /* Left and right */
    for (y = y1; y <= y2; y++) {
        text_40_plot(x1, y);
        text_40_plot(x2, y);
    }
}

static void text_40_circle(int x, int y, int radius) {
    /* Simple circle approximation in text mode */
    int px, py;
    int i;

    for (i = 0; i < 360; i += 45) {
        /* Simple integer sine/cosine approximation */
        int sine_val = 0, cos_val = radius;

        if (i >= 45 && i < 135) {
            sine_val = radius;
        } else if (i >= 135 && i < 225) {
            sine_val = radius;
            cos_val = -radius;
        } else if (i >= 225 && i < 315) {
            cos_val = -radius;
        }

        px = x + cos_val;
        py = y + sine_val;
        text_40_plot(px, py);
    }
}

static void text_40_clear(void) {
    clrscr();
}

/* ============================================================================
 * DRIVER STRUCT
 * ============================================================================ */

graphics_driver_t graphics_driver_text_40x25 = {
    .name = "TEXT_40x25",
    .mode = GRAPHICS_MODE_TEXT_40x25,
    .width = 40,
    .height = 25,
    .max_colors = 16,

    .init = text_40_init,
    .done = text_40_done,
    .setcolor = text_40_setcolor,
    .setbkcolor = text_40_setbkcolor,
    .plot = text_40_plot,
    .getpixel = text_40_getpixel,
    .line = text_40_line,
    .bar = text_40_bar,
    .rect = text_40_rect,
    .circle = text_40_circle,
    .clear = text_40_clear
};

