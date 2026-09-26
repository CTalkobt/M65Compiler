/* graphics_hal.c — VIC-IV Hardware Abstraction Layer Implementation
 *
 * VIC-IV register access, banking, memory management for MEGA65 graphics.
 */

#include <graphics_hal.h>
#include <mega65.h>
#include <string.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

graphics_hal_state_t graphics_hal_state = {
    .screen_addr = 0x0400,
    .color_addr = 0xD800,
    .current_bank = 0,
    .bitmap_mode = 0,
    .multicolor_mode = 0,
    .clip_x1 = 0,
    .clip_y1 = 0,
    .clip_x2 = 319,
    .clip_y2 = 199,
    .screen_width = 320,
    .screen_height = 200
};

/* ============================================================================
 * REGISTER ACCESS (Low-Level)
 * ============================================================================ */

void graphics_write_reg(unsigned char reg, unsigned char value) {
    VIC_REGISTERS[reg] = value;
}

unsigned char graphics_read_reg(unsigned char reg) {
    return VIC_REGISTERS[reg];
}

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

void graphics_hal_init(void) {
    graphics_hal_state.screen_addr = 0x0400;
    graphics_hal_state.color_addr = 0xD800;
    graphics_hal_state.current_bank = 0;
    graphics_hal_state.bitmap_mode = 0;
    graphics_hal_state.multicolor_mode = 0;
    graphics_hal_state.clip_x1 = 0;
    graphics_hal_state.clip_y1 = 0;
    graphics_hal_state.clip_x2 = 319;
    graphics_hal_state.clip_y2 = 199;
    graphics_hal_state.screen_width = 320;
    graphics_hal_state.screen_height = 200;
}

void graphics_hal_done(void) {
    graphics_disable_bitmap_mode();
    graphics_set_bank(0);
}

/* ============================================================================
 * BANK MANAGEMENT
 * ============================================================================ */

void graphics_set_bank(unsigned char bank) {
    unsigned char port_a;

    if (bank > 7) bank = 7;
    graphics_hal_state.current_bank = bank;

    port_a = CIA2_REGISTERS[0];
    port_a = (port_a & 0xFC) | (bank & 0x03);
    CIA2_REGISTERS[0] = port_a;
}

unsigned char graphics_get_bank(void) {
    return graphics_hal_state.current_bank;
}

void graphics_set_display_addr(unsigned int addr) {
    unsigned char mem_setup;

    graphics_hal_state.screen_addr = addr;
    mem_setup = graphics_read_reg(VICIV_SCREEN_ADDR);
    mem_setup = (mem_setup & 0x0F) | ((addr >> 10) & 0xF0);
    graphics_write_reg(VICIV_SCREEN_ADDR, mem_setup);
}

void graphics_set_bitmap_addr(unsigned int addr) {
    unsigned char mem_setup;

    mem_setup = graphics_read_reg(VICIV_SCREEN_ADDR);
    mem_setup = (mem_setup & 0x0F) | ((addr >> 10) & 0xF0);
    graphics_write_reg(VICIV_SCREEN_ADDR, mem_setup);
}

void graphics_set_color_addr(unsigned int addr) {
    graphics_hal_state.color_addr = addr;
}

/* ============================================================================
 * MODE CONTROL
 * ============================================================================ */

void graphics_enable_bitmap_mode(void) {
    unsigned char mode_0;

    graphics_hal_state.bitmap_mode = 1;
    mode_0 = graphics_read_reg(VICIV_MODE_CONTROL_0);
    mode_0 |= VICIV_BMM;
    graphics_write_reg(VICIV_MODE_CONTROL_0, mode_0);

    graphics_set_bitmap_addr(0x2000);
    graphics_set_display_addr(0x0400);

    graphics_hal_state.screen_width = 320;
    graphics_hal_state.screen_height = 200;
    graphics_hal_state.clip_x2 = 319;
    graphics_hal_state.clip_y2 = 199;
}

void graphics_disable_bitmap_mode(void) {
    unsigned char mode_0;

    graphics_hal_state.bitmap_mode = 0;
    mode_0 = graphics_read_reg(VICIV_MODE_CONTROL_0);
    mode_0 &= ~VICIV_BMM;
    graphics_write_reg(VICIV_MODE_CONTROL_0, mode_0);
}

void graphics_enable_multicolor(void) {
    unsigned char mode_1;

    graphics_hal_state.multicolor_mode = 1;
    mode_1 = graphics_read_reg(VICIV_MODE_CONTROL_1);
    mode_1 |= VICIV_MCM;
    graphics_write_reg(VICIV_MODE_CONTROL_1, mode_1);

    graphics_hal_state.screen_width = 160;
    graphics_hal_state.screen_height = 200;
}

void graphics_disable_multicolor(void) {
    unsigned char mode_1;

    graphics_hal_state.multicolor_mode = 0;
    mode_1 = graphics_read_reg(VICIV_MODE_CONTROL_1);
    mode_1 &= ~VICIV_MCM;
    graphics_write_reg(VICIV_MODE_CONTROL_1, mode_1);
}

void graphics_enable_extended_text(void) {
    unsigned char mode_2;

    mode_2 = graphics_read_reg(VICIV_MODE_CONTROL_2);
    mode_2 |= VICIV_EXTENDED_TEXT_MODE;
    graphics_write_reg(VICIV_MODE_CONTROL_2, mode_2);

    graphics_hal_state.screen_width = 80;
    graphics_hal_state.screen_height = 24;
}

void graphics_disable_extended_text(void) {
    unsigned char mode_2;

    mode_2 = graphics_read_reg(VICIV_MODE_CONTROL_2);
    mode_2 &= ~VICIV_EXTENDED_TEXT_MODE;
    graphics_write_reg(VICIV_MODE_CONTROL_2, mode_2);

    graphics_hal_state.screen_width = 40;
    graphics_hal_state.screen_height = 25;
}

/* ============================================================================
 * BUFFER ACCESS
 * ============================================================================ */

unsigned char *graphics_get_screen_buffer(void) {
    return GRAPHICS_SCREEN_RAM;
}

unsigned char *graphics_get_color_buffer(void) {
    return GRAPHICS_COLOR_RAM;
}

void graphics_fill_rect(unsigned int addr, unsigned int len, unsigned char val) {
    unsigned int i;
    unsigned char *ptr = (unsigned char *)addr;

    for (i = 0; i < len; i++) {
        ptr[i] = val;
    }
}

void graphics_copy_rect(unsigned int dest, unsigned int src, unsigned int len) {
    unsigned int i;
    unsigned char *d = (unsigned char *)dest;
    unsigned char *s = (unsigned char *)src;

    if (dest < src) {
        for (i = 0; i < len; i++) {
            d[i] = s[i];
        }
    } else {
        for (i = len; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
}

/* ============================================================================
 * CLIPPING & BOUNDS CHECKING
 * ============================================================================ */

void graphics_set_clip(int x1, int y1, int x2, int y2) {
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }

    graphics_hal_state.clip_x1 = x1;
    graphics_hal_state.clip_y1 = y1;
    graphics_hal_state.clip_x2 = x2;
    graphics_hal_state.clip_y2 = y2;
}

void graphics_clear_clip(void) {
    graphics_hal_state.clip_x1 = 0;
    graphics_hal_state.clip_y1 = 0;
    graphics_hal_state.clip_x2 = graphics_hal_state.screen_width - 1;
    graphics_hal_state.clip_y2 = graphics_hal_state.screen_height - 1;
}

int graphics_clip_x(int x) {
    if (x < graphics_hal_state.clip_x1 || x > graphics_hal_state.clip_x2) {
        return -1;
    }
    return x;
}

int graphics_clip_y(int y) {
    if (y < graphics_hal_state.clip_y1 || y > graphics_hal_state.clip_y2) {
        return -1;
    }
    return y;
}

int graphics_clip_point(int x, int y) {
    return (x >= graphics_hal_state.clip_x1 && x <= graphics_hal_state.clip_x2 &&
            y >= graphics_hal_state.clip_y1 && y <= graphics_hal_state.clip_y2);
}

int graphics_clip_line(int x1, int y1, int x2, int y2,
                       int *cx1, int *cy1, int *cx2, int *cy2) {
    int x_min = graphics_hal_state.clip_x1;
    int y_min = graphics_hal_state.clip_y1;
    int x_max = graphics_hal_state.clip_x2;
    int y_max = graphics_hal_state.clip_y2;
    int code1 = 0, code2 = 0;

    if (x1 < x_min) code1 |= 1;
    if (x1 > x_max) code1 |= 2;
    if (y1 < y_min) code1 |= 4;
    if (y1 > y_max) code1 |= 8;

    if (x2 < x_min) code2 |= 1;
    if (x2 > x_max) code2 |= 2;
    if (y2 < y_min) code2 |= 4;
    if (y2 > y_max) code2 |= 8;

    if (code1 == 0 && code2 == 0) {
        *cx1 = x1; *cy1 = y1;
        *cx2 = x2; *cy2 = y2;
        return 1;
    }

    if (code1 & code2) {
        return 0;
    }

    *cx1 = x1; *cy1 = y1;
    *cx2 = x2; *cy2 = y2;
    return 1;
}

