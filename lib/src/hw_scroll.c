/* hw_scroll.c — Hardware Scrolling Implementation */

#include "hw_scroll.h"
#include <stdint.h>
#include <stdlib.h>

/* VIC-IV scroll registers */
#define VIC4_SCREEN_X_LO (*(volatile unsigned char *)0xD040)
#define VIC4_SCREEN_X_HI (*(volatile unsigned char *)0xD041)
#define VIC4_SCREEN_Y_LO (*(volatile unsigned char *)0xD042)
#define VIC4_SCREEN_Y_HI (*(volatile unsigned char *)0xD043)

static hw_scroll_state_t scroll_state = {0};
static int smooth_target_x = 0, smooth_target_y = 0;
static int smooth_current_x = 0, smooth_current_y = 0;
static int smooth_duration = 0, smooth_elapsed = 0;
static int smooth_active = 0;
static int parallax_offsets[4][2] = {{0}};

void hw_scroll_init(void) {
    scroll_state.x = 0;
    scroll_state.y = 0;
    scroll_state.fine_x = 0;
    scroll_state.fine_y = 0;
    smooth_active = 0;
}

void hw_scroll_set_x(int pixels) {
    scroll_state.x = pixels;
    scroll_state.fine_x = 0;

    /* Write to VIC-IV scroll registers (16-bit X) */
    VIC4_SCREEN_X_LO = (unsigned char)(pixels & 0xFF);
    VIC4_SCREEN_X_HI = (unsigned char)((pixels >> 8) & 0xFF);
}

void hw_scroll_set_y(int pixels) {
    scroll_state.y = pixels;
    scroll_state.fine_y = 0;

    /* Write to VIC-IV scroll registers (16-bit Y) */
    VIC4_SCREEN_Y_LO = (unsigned char)(pixels & 0xFF);
    VIC4_SCREEN_Y_HI = (unsigned char)((pixels >> 8) & 0xFF);
}

void hw_scroll_set_fine_x(int subpixels) {
    if (subpixels < 0) subpixels = 0;
    if (subpixels > 7) subpixels = 7;
    scroll_state.fine_x = subpixels;

    /* Fine X scroll via CTRL2 bits (if supported) */
    volatile unsigned char *ctrl2 = (volatile unsigned char *)0xD016;
    *ctrl2 = (*ctrl2 & 0xF8) | (subpixels & 0x07);
}

void hw_scroll_set_fine_y(int subpixels) {
    if (subpixels < 0) subpixels = 0;
    if (subpixels > 7) subpixels = 7;
    scroll_state.fine_y = subpixels;

    /* Fine Y scroll via CTRL1 bits (if supported) */
    volatile unsigned char *ctrl1 = (volatile unsigned char *)0xD011;
    *ctrl1 = (*ctrl1 & 0xF8) | (subpixels & 0x07);
}

hw_scroll_state_t hw_scroll_get_state(void) {
    return scroll_state;
}

void hw_scroll_to(int target_x, int target_y, int duration_ms) {
    smooth_target_x = target_x;
    smooth_target_y = target_y;
    smooth_current_x = scroll_state.x;
    smooth_current_y = scroll_state.y;
    smooth_duration = duration_ms;
    smooth_elapsed = 0;
    smooth_active = 1;
}

int hw_scroll_update(int delta_ms) {
    if (!smooth_active) return 0;

    smooth_elapsed += delta_ms;
    if (smooth_elapsed >= smooth_duration) {
        /* Snap to target */
        hw_scroll_set_x(smooth_target_x);
        hw_scroll_set_y(smooth_target_y);
        smooth_active = 0;
        return 1;
    }

    /* Linear interpolation */
    float t = (float)smooth_elapsed / (float)smooth_duration;
    int new_x = smooth_current_x + (int)((smooth_target_x - smooth_current_x) * t);
    int new_y = smooth_current_y + (int)((smooth_target_y - smooth_current_y) * t);

    hw_scroll_set_x(new_x);
    hw_scroll_set_y(new_y);

    return 1;
}

void hw_scroll_set_parallax(int layer, int offset_x, int offset_y) {
    if (layer < 0 || layer > 3) return;
    parallax_offsets[layer][0] = offset_x;
    parallax_offsets[layer][1] = offset_y;
}

void hw_scroll_reset(void) {
    hw_scroll_set_x(0);
    hw_scroll_set_y(0);
    hw_scroll_set_fine_x(0);
    hw_scroll_set_fine_y(0);
    smooth_active = 0;
}

int hw_scroll_is_active(void) {
    return smooth_active;
}
