/* hw_raster.h — raster interrupts and timing (#645)
 *
 * hw_raster_init puts the library's handler in the KERNAL's IRQ vector
 * ($0314; the KERNAL's back at exit, or hw_raster_done). It keeps a table of
 * raster lines, sorted, and sets the VIC's raster compare to each in turn:
 *
 *   the KERNAL's line  (255, hw_raster_set_line moves it): the KERNAL's own
 *                      handler runs there once a frame (keyboard, cursor,
 *                      clock()); hw_raster_get_frame counts it
 *   up to HW_RASTER_MAX lines of the program's, each with
 *     hw_raster_set_isr       an __interrupt function, entered as the CPU
 *                             would enter it (its RTI returns to the
 *                             library, which returns from the interrupt);
 *                             ln45 checks what it reaches, as for any
 *                             __interrupt handler
 *     hw_raster_set_callback  or a plain function, void f(int line, void
 *                             *user), called from the library's own
 *                             __interrupt handler (stdlib/hw_raster_cb.c);
 *                             reached through a pointer from an interrupt,
 *                             so ln45 wants every function of that type
 *                             whose address is taken to have a stack frame
 *                             (#pragma cc45 recurse), not static allocation
 *   (a program line that is the KERNAL's too: the program's first, then the
 *   KERNAL's)
 *
 * The handler and its table are in the "init" segment (ln45 places it
 * first, below $4000): the float routines map the ROM over $4000-$7FFF
 * while they run (float_map.s45), and an interrupt then must find them. A
 * program's own __interrupt function at $4000-$7FFF must not run then either.
 *
 * Lines are the VIC-II's: 0-311 (PAL) or 0-262 (NTSC); the 25 text rows of
 * the standard screen are lines 51-250. hw_raster_disable / _enable turn the
 * program's lines off and on (the KERNAL's keeps running: masking the raster
 * interrupt would stop the keyboard and clock()).
 *
 * All __fastcall__ (the ZP convention in either library). Library:
 * stdlib_asm/hw_raster.s45, stdlib/hw_raster_cb.c. Test: test_hw_raster_xemu.c.
 */

#ifndef HW_RASTER_H
#define HW_RASTER_H

#define HW_RASTER_MAX           8       /* the program's lines */
#define HW_RASTER_KERNAL_LINE   255     /* the KERNAL's, at first */

typedef void (*raster_callback_t)(int raster_line, void *user_data);

/* The library's handler in the IRQ vector (again: nothing more); the
 * program's lines cleared, the KERNAL's at 255 */
__fastcall__ void hw_raster_init(void);

/* The KERNAL's vector back, the compare at the KERNAL's line */
__fastcall__ void hw_raster_done(void);

/* The KERNAL's interrupt at this line (0-311 / 0-262): 1, 0 out of range.
 * Before hw_raster_init: the VIC's raster compare, the KERNAL's handler */
__fastcall__ int hw_raster_set_line(int line);

/* The raster line now */
__fastcall__ int hw_raster_get_line(void);

/* An __interrupt function at a line (replacing the line's): 1; 0 when the
 * line is out of range or HW_RASTER_MAX lines are taken (or no
 * hw_raster_init) */
__fastcall__ int hw_raster_set_isr(int line, void (*isr)(void));

/* A plain function at a line, called with the line and user_data: as
 * hw_raster_set_isr */
__fastcall__ int hw_raster_set_callback(int line, raster_callback_t callback, void *user_data);

/* The line's function off (either kind) */
__fastcall__ void hw_raster_clear_callback(int line);

/* The line the interrupt being handled is for (in a line's function) */
__fastcall__ int hw_raster_current(void);

/* The VIC-IV's horizontal position in the line (XPOS, $D050/$D051: 0-16383) */
__fastcall__ int hw_raster_get_x(void);

/* 1 while the raster is outside the standard screen's text rows (lines
 * 251 to the end of the frame, 0-50): a time to change what is shown */
__fastcall__ int hw_raster_in_vblank(void);

/* Wait for the line to begin (the next time: on it now, the one after) */
__fastcall__ void hw_raster_wait_line(int line);

/* Wait for the line after the text rows (251) to begin */
__fastcall__ void hw_raster_wait_vblank(void);

/* Frames since hw_raster_init (the KERNAL's line each frame), 16 bits */
__fastcall__ int hw_raster_get_frame(void);

/* The program's lines off / on again (the KERNAL's keeps running) */
__fastcall__ void hw_raster_disable(void);
__fastcall__ void hw_raster_enable(void);

#endif
