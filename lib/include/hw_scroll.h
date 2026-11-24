/* hw_scroll.h — hardware scrolling of the text screen (#645)
 *
 * hw_scroll_init takes the screen as it is as the origin: the screen and
 * colour pointers ($D060-$D063, $D064/$D065) and the text position
 * ($D04C/$D04D X, $D04E/$D04F Y). A position (x, y) in pixels then shows
 * the screen memory from there:
 *   coarse  x / 8 characters, y / 8 rows: both pointers moved on by
 *           row * LINESTEP ($D058/$D059) + column (two bytes a character
 *           with 16-bit characters, $D054 bit 0)
 *   fine    x % 8, y % 8: the text moved up and left by that many pixels,
 *           the text position less 2 a pixel (1 at 640 wide, $D031 bit 7,
 *           and at 400 lines, bit 3)
 * The memory past the screen's last column and row shows as the view
 * moves: a program scrolling a wider map keeps LINESTEP its width.
 *
 * hw_scroll_to moves smoothly to a position over duration_ms; each
 * hw_scroll_update(delta_ms) steps it (1 while it is still moving, 0 at
 * the end). hw_scroll_reset puts the origin back.
 *
 * A $D011 / $D016 write with the VIC-IV's hot registers on ($D05D bit 7)
 * sets the text position again from those registers: set the position
 * after such a write (hw_scroll_set_x), or turn the hot registers off.
 *
 * Positions are unsigned (0-65535). All __fastcall__ (the ZP convention in
 * either library). Library: stdlib_asm/hw_scroll.s45. Test:
 * test_hw_scroll_xemu.c.
 */

#ifndef HW_SCROLL_H
#define HW_SCROLL_H

typedef struct {
    int x;              /* the position, pixels */
    int y;
    int fine_x;         /* x % 8, y % 8: the fine scroll */
    int fine_y;
} hw_scroll_state_t;

/* The screen as it is now: the origin, the position 0, 0 */
__fastcall__ void hw_scroll_init(void);

/* The position (fine and coarse) */
__fastcall__ void hw_scroll_set_x(int pixels);
__fastcall__ void hw_scroll_set_y(int pixels);

/* The fine scroll only (0-7; the position's character kept) */
__fastcall__ void hw_scroll_set_fine_x(int subpixels);
__fastcall__ void hw_scroll_set_fine_y(int subpixels);

/* Get current scroll state */
__fastcall__ hw_scroll_state_t hw_scroll_get_state(void);

/* Move to a position over duration_ms (0: at once) */
__fastcall__ void hw_scroll_to(int target_x, int target_y, int duration_ms);

/* The move stepped on by delta_ms: 1 while still moving */
__fastcall__ int hw_scroll_update(int delta_ms);

/* The origin again (a move stopped) */
__fastcall__ void hw_scroll_reset(void);

/* 1 while a move goes on */
__fastcall__ int hw_scroll_is_active(void);

#endif
