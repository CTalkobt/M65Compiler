/* hw_raster.c — Raster Effects & Timing Implementation */

#include "hw_raster.h"
#include <stdint.h>
#include <string.h>

/* VIC-IV raster registers */
#define VIC4_RASTER_LO (*(volatile unsigned char *)0xD012)
#define VIC4_RASTER_HI (*(volatile unsigned char *)0xD011)  /* Bit 7 */
#define VIC4_RASTER_HI_NEW (*(volatile unsigned char *)0xD046)
#define VIC4_XPOS_LO (*(volatile unsigned char *)0xD044)
#define VIC4_XPOS_HI (*(volatile unsigned char *)0xD045)
#define VIC4_CTRL1 (*(volatile unsigned char *)0xD011)
#define VIC4_CTRL2 (*(volatile unsigned char *)0xD016)
#define VIC4_IRQ_STATUS (*(volatile unsigned char *)0xD019)
#define VIC4_IRQ_ENABLE (*(volatile unsigned char *)0xD01A)

typedef struct {
    raster_callback_t callback;
    void *user_data;
    int active;
} raster_irq_t;

static raster_irq_t raster_interrupts[400] = {{0}};
static int current_line = 0;
static int frame_counter = 0;
static unsigned int cycle_counter = 0;
static int raster_enabled = 0;

void hw_raster_init(void) {
    /* Initialize raster interrupt table */
    memset(raster_interrupts, 0, sizeof(raster_interrupts));
    frame_counter = 0;
    cycle_counter = 0;
    raster_enabled = 0;

    /* Enable raster interrupt in VIC-IV */
    VIC4_IRQ_ENABLE |= 0x01;  /* Raster IRQ enable */
}

int hw_raster_set_line(int line) {
    if (line < 0 || line > 399) return 0;

    current_line = line;

    /* Set raster line (9-bit: low byte + high bit) */
    VIC4_RASTER_LO = (unsigned char)(line & 0xFF);

    /* High bit goes to $D046 or $D011 bit 7 depending on VIC version */
    if (line > 255) {
        VIC4_RASTER_HI_NEW = (unsigned char)((line >> 8) & 0x01);
    }

    return 1;
}

int hw_raster_get_line(void) {
    /* Read current raster line */
    int lo = VIC4_RASTER_LO;
    int hi = (VIC4_RASTER_HI_NEW & 0x01) << 8;
    return lo | hi;
}

int hw_raster_set_callback(int line, raster_callback_t callback, void *user_data) {
    if (line < 0 || line > 399) return 0;

    raster_interrupts[line].callback = callback;
    raster_interrupts[line].user_data = user_data;
    raster_interrupts[line].active = 1;

    return 1;
}

void hw_raster_clear_callback(int line) {
    if (line < 0 || line > 399) return;
    raster_interrupts[line].active = 0;
}

int hw_raster_get_x(void) {
    /* Read X position within scanline (cycle) */
    int lo = VIC4_XPOS_LO;
    int hi = VIC4_XPOS_HI;
    return lo | (hi << 8);
}

int hw_raster_in_hblank(void) {
    /* Approximate hblank detection based on X position */
    int x = hw_raster_get_x();
    return x >= 320;  /* Rough estimate */
}

int hw_raster_in_vblank(void) {
    /* Approximate vblank detection based on raster line */
    int line = hw_raster_get_line();
    return line >= 250;  /* Rough estimate for NTSC/PAL */
}

void hw_raster_wait_line(int line) {
    if (line < 0 || line > 399) return;

    /* Busy-wait for raster line (not ideal but works) */
    while (hw_raster_get_line() != line) {
        /* Spin */
    }
}

void hw_raster_wait_vblank(void) {
    /* Wait for vertical blank period */
    int initial_frame = frame_counter;
    while (frame_counter == initial_frame) {
        /* Spin until frame counter increments */
    }
}

int hw_raster_get_frame(void) {
    return frame_counter;
}

unsigned int hw_raster_get_cycle(void) {
    return cycle_counter;
}

void hw_raster_disable(void) {
    VIC4_IRQ_ENABLE &= ~0x01;
    raster_enabled = 0;
}

void hw_raster_enable(void) {
    VIC4_IRQ_ENABLE |= 0x01;
    raster_enabled = 1;
}

/* Called on raster interrupt (typically from ISR) */
void hw_raster_handle_interrupt(void) {
    if (!raster_enabled) return;

    int line = hw_raster_get_line();
    cycle_counter++;

    /* Check for frame boundary */
    if (line == 0) {
        frame_counter++;
    }

    /* Dispatch callback if registered */
    if (line >= 0 && line < 400 && raster_interrupts[line].active) {
        if (raster_interrupts[line].callback) {
            raster_interrupts[line].callback(line, raster_interrupts[line].user_data);
        }
    }

    /* Acknowledge interrupt */
    VIC4_IRQ_STATUS = 0x01;
}
