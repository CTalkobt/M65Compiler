/* FOR THE FUTURE: NOT IMPLEMENTED.
 *
 * No library defines what this header declares: a call of one of its
 * functions would compile and then fail to link. Its contents are kept for
 * later work and are left out unless CC45_FUTURE is defined (-DCC45_FUTURE),
 * so including it by accident declares nothing.
 * (doc/architecture/stdlib.md, "Headers Without a Library Implementation")
 */
#ifdef CC45_FUTURE

/* hw_scroll.h — Phase 82: Hardware Scrolling
 *
 * Hardware-accelerated X/Y scrolling with fine scroll control,
 * parallax support, and tile-based alignment.
 */

#ifndef HW_SCROLL_H
#define HW_SCROLL_H

typedef struct {
    int x;              /* Coarse X scroll (pixels) */
    int y;              /* Coarse Y scroll (pixels) */
    int fine_x;         /* Fine X scroll (0-7 subpixels) */
    int fine_y;         /* Fine Y scroll (0-7 subpixels) */
    int parallax_x;     /* Parallax layer offset */
    int parallax_y;
} hw_scroll_state_t;

/* Initialize hardware scrolling */
void hw_scroll_init(void);

/* Set coarse scroll position */
void hw_scroll_set_x(int pixels);
void hw_scroll_set_y(int pixels);

/* Set fine scroll (subpixel, 0-7) */
void hw_scroll_set_fine_x(int subpixels);
void hw_scroll_set_fine_y(int subpixels);

/* Get current scroll state */
hw_scroll_state_t hw_scroll_get_state(void);

/* Smooth scrolling (animate to target over time) */
void hw_scroll_to(int target_x, int target_y, int duration_ms);

/* Update smooth scrolling (call each frame) */
int hw_scroll_update(int delta_ms);

/* Set parallax offset for background layers */
void hw_scroll_set_parallax(int layer, int offset_x, int offset_y);

/* Reset scrolling to origin */
void hw_scroll_reset(void);

/* Check if scrolling in progress */
int hw_scroll_is_active(void);

#endif

#endif /* CC45_FUTURE */
