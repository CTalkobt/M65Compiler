/* FOR THE FUTURE: NOT IMPLEMENTED.
 *
 * No library defines what this header declares: a call of one of its
 * functions would compile and then fail to link. Its contents are kept for
 * later work and are left out unless CC45_FUTURE is defined (-DCC45_FUTURE),
 * so including it by accident declares nothing.
 * (doc/architecture/stdlib.md, "Headers Without a Library Implementation")
 */
#ifdef CC45_FUTURE

/* hw_raster.h — Phase 85: Raster Effects & Timing
 *
 * Raster interrupt handling, cycle-accurate timing, and effects
 * that operate per-scanline on MEGA65.
 */

#ifndef HW_RASTER_H
#define HW_RASTER_H

typedef void (*raster_callback_t)(int raster_line, void *user_data);

/* Initialize raster interrupt system */
void hw_raster_init(void);

/* Set raster interrupt on specific line (0-399) */
int hw_raster_set_line(int line);

/* Get current raster line */
int hw_raster_get_line(void);

/* Register callback for raster line */
int hw_raster_set_callback(int line, raster_callback_t callback, void *user_data);

/* Remove raster callback */
void hw_raster_clear_callback(int line);

/* Get raster X position (cycle within scanline) */
int hw_raster_get_x(void);

/* Horizontal blank state (1 during hblank, 0 during visible pixels) */
int hw_raster_in_hblank(void);

/* Vertical blank state */
int hw_raster_in_vblank(void);

/* Wait for specific raster line (blocking) */
void hw_raster_wait_line(int line);

/* Wait for vblank (frame sync) */
void hw_raster_wait_vblank(void);

/* Get frame counter (increments on vblank) */
int hw_raster_get_frame(void);

/* Get CPU cycle counter (approximate) */
unsigned int hw_raster_get_cycle(void);

/* Disable raster interrupts */
void hw_raster_disable(void);

/* Enable raster interrupts */
void hw_raster_enable(void);

#endif

#endif /* CC45_FUTURE */
