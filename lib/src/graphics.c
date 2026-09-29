/* graphics.c — Graphics Framework Implementation
 *
 * Device-independent graphics abstraction with pluggable drivers.
 * Dispatches calls to active driver based on current mode.
 */

#include <graphics.h>
#include <graphics_hal.h>

/* ============================================================================
 * FORWARD DECLARATIONS (Drivers implemented in separate files)
 * ============================================================================ */

extern graphics_driver_t graphics_driver_text_40x25;
extern graphics_driver_t graphics_driver_text_80x24;
extern graphics_driver_t graphics_driver_graphics_80x50;
extern graphics_driver_t graphics_driver_bitmap_320x200;
extern graphics_driver_t graphics_driver_bitmap_160x200;

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

graphics_state_t graphics_state = {
    .active_driver = 0,
    .current_color = 1,             /* WHITE */
    .current_bkcolor = 0,           /* BLACK */
    .clip_x1 = 0,
    .clip_y1 = 0,
    .clip_x2 = 319,
    .clip_y2 = 199
};

/* ============================================================================
 * DRIVER TABLE
 * ============================================================================ */

static graphics_driver_t *drivers[] = {
    &graphics_driver_text_40x25,
    &graphics_driver_text_80x24,
    &graphics_driver_graphics_80x50,
    &graphics_driver_bitmap_320x200,
    &graphics_driver_bitmap_160x200,
    0  /* Sentinel */
};

/* ============================================================================
 * FRAMEWORK INITIALIZATION
 * ============================================================================ */

void graphics_init(void) {
    graphics_hal_init();

    /* Initialize to TEXT_40x25 mode by default */
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
}

void graphics_done(void) {
    if (graphics_state.active_driver && graphics_state.active_driver->done) {
        graphics_state.active_driver->done();
    }
    graphics_hal_done();
}

/* ============================================================================
 * MODE MANAGEMENT
 * ============================================================================ */

int graphics_setmode(graphics_mode_t mode) {
    graphics_driver_t *driver = 0;
    int i;

    /* Find driver for requested mode */
    for (i = 0; drivers[i] != 0; i++) {
        if (drivers[i]->mode == mode) {
            driver = drivers[i];
            break;
        }
    }

    if (!driver) {
        return -1;  /* Mode not supported */
    }

    /* Clean up previous driver */
    if (graphics_state.active_driver && graphics_state.active_driver->done) {
        graphics_state.active_driver->done();
    }

    /* Switch to new driver */
    graphics_state.active_driver = driver;
    if (driver->init) {
        driver->init();
    }

    /* Update clipping to match new mode dimensions */
    graphics_state.clip_x1 = 0;
    graphics_state.clip_y1 = 0;
    graphics_state.clip_x2 = driver->width - 1;
    graphics_state.clip_y2 = driver->height - 1;

    return 0;
}

graphics_mode_t graphics_getmode(void) {
    if (graphics_state.active_driver) {
        return graphics_state.active_driver->mode;
    }
    return GRAPHICS_MODE_TEXT_40x25;
}

int graphics_getwidth(void) {
    if (graphics_state.active_driver) {
        return graphics_state.active_driver->width;
    }
    return 0;
}

int graphics_getheight(void) {
    if (graphics_state.active_driver) {
        return graphics_state.active_driver->height;
    }
    return 0;
}

int graphics_getmaxcolor(void) {
    if (graphics_state.active_driver) {
        return graphics_state.active_driver->max_colors - 1;
    }
    return 0;
}

/* ============================================================================
 * COLOR CONTROL
 * ============================================================================ */

void graphics_setcolor(unsigned char color) {
    graphics_state.current_color = color;

    if (graphics_state.active_driver && graphics_state.active_driver->setcolor) {
        graphics_state.active_driver->setcolor(color);
    }
}

void graphics_setbkcolor(unsigned char color) {
    graphics_state.current_bkcolor = color;

    if (graphics_state.active_driver && graphics_state.active_driver->setbkcolor) {
        graphics_state.active_driver->setbkcolor(color);
    }
}

unsigned char graphics_getcolor(void) {
    return graphics_state.current_color;
}

unsigned char graphics_getbkcolor(void) {
    return graphics_state.current_bkcolor;
}

/* ============================================================================
 * DRAWING PRIMITIVES (Dispatch to active driver)
 * ============================================================================ */

void graphics_plot(int x, int y) {
    if (graphics_state.active_driver && graphics_state.active_driver->plot) {
        /* TODO: Apply clipping */
        graphics_state.active_driver->plot(x, y);
    }
}

unsigned char graphics_getpixel(int x, int y) {
    if (graphics_state.active_driver && graphics_state.active_driver->getpixel) {
        return graphics_state.active_driver->getpixel(x, y);
    }
    return 0;
}

void graphics_line(int x1, int y1, int x2, int y2) {
    if (graphics_state.active_driver && graphics_state.active_driver->line) {
        /* TODO: Apply clipping (Cohen-Sutherland) */
        graphics_state.active_driver->line(x1, y1, x2, y2);
    }
}

void graphics_bar(int x1, int y1, int x2, int y2) {
    if (graphics_state.active_driver && graphics_state.active_driver->bar) {
        /* TODO: Apply clipping */
        graphics_state.active_driver->bar(x1, y1, x2, y2);
    }
}

void graphics_rect(int x1, int y1, int x2, int y2) {
    if (graphics_state.active_driver && graphics_state.active_driver->rect) {
        /* TODO: Apply clipping */
        graphics_state.active_driver->rect(x1, y1, x2, y2);
    }
}

void graphics_circle(int x, int y, int radius) {
    if (graphics_state.active_driver && graphics_state.active_driver->circle) {
        /* TODO: Apply clipping */
        graphics_state.active_driver->circle(x, y, radius);
    }
}

void graphics_clear(void) {
    if (graphics_state.active_driver && graphics_state.active_driver->clear) {
        graphics_state.active_driver->clear();
    }
}

/* ============================================================================
 * CLIPPING CONTROL
 * ============================================================================ */

void graphics_setclip(int x1, int y1, int x2, int y2) {
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    graphics_state.clip_x1 = x1;
    graphics_state.clip_y1 = y1;
    graphics_state.clip_x2 = x2;
    graphics_state.clip_y2 = y2;

    /* Also update HAL clipping */
    graphics_set_clip(x1, y1, x2, y2);
}

void graphics_clearclip(void) {
    if (graphics_state.active_driver) {
        graphics_state.clip_x1 = 0;
        graphics_state.clip_y1 = 0;
        graphics_state.clip_x2 = graphics_state.active_driver->width - 1;
        graphics_state.clip_y2 = graphics_state.active_driver->height - 1;

        graphics_clear_clip();
    }
}

void graphics_getclip(int *x1, int *y1, int *x2, int *y2) {
    *x1 = graphics_state.clip_x1;
    *y1 = graphics_state.clip_y1;
    *x2 = graphics_state.clip_x2;
    *y2 = graphics_state.clip_y2;
}

