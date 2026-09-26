/* graphics_text80x50.c — TEXT_80x50 Graphics Mode Implementation (Phase 6)
 *
 * 80×50 extended text mode with drawing primitives.
 */

#include <graphics_text80x50.h>
#include <string.h>

/* ============================================================================
 * STATE VARIABLES
 * ============================================================================ */

static unsigned char fg_color = COLOR_WHITE;
static unsigned char bg_color = COLOR_BLACK;
static int cursor_x = 0;
static int cursor_y = 0;
static int cursor_visible = 1;

/* ============================================================================
 * MEMORY ACCESS HELPERS
 * ============================================================================ */

static inline int text80x50_offset(int x, int y) {
    if (x < 0 || x >= TEXT80X50_WIDTH || y < 0 || y >= TEXT80X50_HEIGHT) {
        return -1;
    }
    return y * TEXT80X50_WIDTH + x;
}

static unsigned char *screen_ptr(int x, int y) {
    int offset = text80x50_offset(x, y);
    if (offset < 0) return NULL;
    return (unsigned char *)(TEXT80X50_SCREEN_ADDR + offset);
}

static unsigned char *color_ptr(int x, int y) {
    int offset = text80x50_offset(x, y);
    if (offset < 0) return NULL;
    return (unsigned char *)(TEXT80X50_COLOR_ADDR + offset);
}

/* ============================================================================
 * MODE INITIALIZATION
 * ============================================================================ */

void text80x50_init(void) {
    /* Initialize screen memory */
    unsigned char *screen = (unsigned char *)TEXT80X50_SCREEN_ADDR;
    unsigned char *color = (unsigned char *)TEXT80X50_COLOR_ADDR;

    for (int i = 0; i < TEXT80X50_BUFFER_SIZE; i++) {
        screen[i] = 0x20;  /* Space */
        color[i] = COLOR_WHITE;
    }

    fg_color = COLOR_WHITE;
    bg_color = COLOR_BLACK;
    cursor_x = 0;
    cursor_y = 0;
    cursor_visible = 1;
}

void text80x50_done(void) {
    /* Cleanup if needed */
    text80x50_clear(0x20, COLOR_BLACK);
}

/* ============================================================================
 * CHARACTER & COLOR OPERATIONS
 * ============================================================================ */

void text80x50_putchar(int x, int y, unsigned char ch, unsigned char color) {
    unsigned char *sp = screen_ptr(x, y);
    unsigned char *cp = color_ptr(x, y);

    if (sp && cp) {
        *sp = ch;
        *cp = (color & 0x0F);  /* Clamp to 4 bits */
    }
}

unsigned char text80x50_getchar(int x, int y) {
    unsigned char *sp = screen_ptr(x, y);
    return sp ? *sp : 0;
}

unsigned char text80x50_getcolor(int x, int y) {
    unsigned char *cp = color_ptr(x, y);
    return cp ? *cp : 0;
}

void text80x50_setcolor(int x, int y, unsigned char color) {
    unsigned char *cp = color_ptr(x, y);
    if (cp) *cp = (color & 0x0F);
}

void text80x50_clear(unsigned char ch, unsigned char color) {
    unsigned char *screen = (unsigned char *)TEXT80X50_SCREEN_ADDR;
    unsigned char *clr = (unsigned char *)TEXT80X50_COLOR_ADDR;

    for (int i = 0; i < TEXT80X50_BUFFER_SIZE; i++) {
        screen[i] = ch;
        clr[i] = (color & 0x0F);
    }
}

void text80x50_setfgcolor(unsigned char color) {
    fg_color = (color & 0x0F);
}

void text80x50_setbgcolor(unsigned char color) {
    bg_color = (color & 0x0F);
}

unsigned char text80x50_getfgcolor(void) {
    return fg_color;
}

unsigned char text80x50_getbgcolor(void) {
    return bg_color;
}

/* ============================================================================
 * DRAWING PRIMITIVES
 * ============================================================================ */

void text80x50_hline(int x1, int y, int x2, unsigned char ch, unsigned char color) {
    if (y < 0 || y >= TEXT80X50_HEIGHT) return;

    if (x1 > x2) {
        int tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if (x1 < 0) x1 = 0;
    if (x2 >= TEXT80X50_WIDTH) x2 = TEXT80X50_WIDTH - 1;

    for (int x = x1; x <= x2; x++) {
        text80x50_putchar(x, y, ch, color);
    }
}

void text80x50_vline(int x, int y1, int y2, unsigned char ch, unsigned char color) {
    if (x < 0 || x >= TEXT80X50_WIDTH) return;

    if (y1 > y2) {
        int tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    if (y1 < 0) y1 = 0;
    if (y2 >= TEXT80X50_HEIGHT) y2 = TEXT80X50_HEIGHT - 1;

    for (int y = y1; y <= y2; y++) {
        text80x50_putchar(x, y, ch, color);
    }
}

void text80x50_rect(int x1, int y1, int x2, int y2, unsigned char ch, unsigned char color) {
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }

    /* Clamp to bounds */
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= TEXT80X50_WIDTH) x2 = TEXT80X50_WIDTH - 1;
    if (y2 >= TEXT80X50_HEIGHT) y2 = TEXT80X50_HEIGHT - 1;

    /* Top and bottom edges */
    text80x50_hline(x1, y1, x2, ch, color);
    if (y2 > y1) {
        text80x50_hline(x1, y2, x2, ch, color);
    }

    /* Left and right edges */
    text80x50_vline(x1, y1 + 1, y2 - 1, ch, color);
    if (x2 > x1) {
        text80x50_vline(x2, y1 + 1, y2 - 1, ch, color);
    }
}

void text80x50_fillrect(int x1, int y1, int x2, int y2, unsigned char ch, unsigned char color) {
    if (x1 > x2) { int tmp = x1; x1 = x2; x2 = tmp; }
    if (y1 > y2) { int tmp = y1; y1 = y2; y2 = tmp; }

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= TEXT80X50_WIDTH) x2 = TEXT80X50_WIDTH - 1;
    if (y2 >= TEXT80X50_HEIGHT) y2 = TEXT80X50_HEIGHT - 1;

    for (int y = y1; y <= y2; y++) {
        text80x50_hline(x1, y, x2, ch, color);
    }
}

int text80x50_print(int x, int y, const char *str, unsigned char color) {
    if (!str) return x;

    while (*str && x < TEXT80X50_WIDTH) {
        text80x50_putchar(x, y, *str, color);
        x++;
        str++;
    }

    return x;
}

void text80x50_box(int x1, int y1, int x2, int y2,
                   unsigned char border_ch, unsigned char fill_ch,
                   unsigned char border_color, unsigned char fill_color) {
    /* Fill interior */
    if (x1 + 1 <= x2 - 1 && y1 + 1 <= y2 - 1) {
        text80x50_fillrect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, fill_ch, fill_color);
    }

    /* Draw border */
    text80x50_rect(x1, y1, x2, y2, border_ch, border_color);
}

/* ============================================================================
 * CURSOR OPERATIONS
 * ============================================================================ */

void text80x50_cursor(int x, int y, int visible) {
    if (x < 0 || x >= TEXT80X50_WIDTH || y < 0 || y >= TEXT80X50_HEIGHT) return;

    cursor_x = x;
    cursor_y = y;
    cursor_visible = visible ? 1 : 0;

    if (visible) {
        text80x50_putchar(x, y, '_', fg_color);
    }
}

void text80x50_get_cursor(int *x, int *y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void text80x50_set_cursor(int x, int y) {
    if (x < 0 || x >= TEXT80X50_WIDTH || y < 0 || y >= TEXT80X50_HEIGHT) return;
    cursor_x = x;
    cursor_y = y;
}
