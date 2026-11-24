/* rrb.h — the VIC-IV's raster rewrite buffer: 16-bit character rows with
 * GOTOX tokens (#645)
 *
 * With 16-bit characters ($D054 bit 0) a cell is two bytes of screen RAM
 * and two of colour RAM, and a cell whose colour byte 0 has bit 4 set is a
 * GOTOX token: the characters after it on the row are drawn from x (its
 * screen bytes' bits 0-9: pixels, doubled at 320 wide), over what is drawn
 * already, so a row can hold layers (text over a background, characters
 * as objects). rrb_setup makes rows of chrcount cells (CHRCOUNT, $D05E and
 * $D063 bits 4-5; LINESTEP = 2 * chrcount bytes) on the screen and colour
 * RAM as they are; the cells' contents are the program's: rrb_putchar,
 * rrb_gotox write one.
 *   RRB_TRANSPARENT  (colour byte 0 bit 7) the characters after the token
 *                    leave their background pixels (what is under shows)
 *   RRB_ROWMASK      (bit 3) colour byte 1 is a row mask: bit n set, the
 *                    characters' pixel row n is drawn
 * rrb_double: DBLRR ($D051 bit 6), the buffer redrawn every second raster
 * line: twice the time for a row's tokens and characters, V200 only.
 * The screen's 25 / 50 rows stay the KERNAL's (its editor knows nothing of
 * 16-bit characters: write the cells yourself). Library: stdlib/rrb.c.
 * Test: test_rrb_xemu.c. (mega65-core viciv.vhdl; Dan Sanderson's "Raster
 * rewrite buffer" notes.)
 */
#pragma once

#define RRB_TRANSPARENT 0x80
#define RRB_ROWMASK     0x08

/* 16-bit characters, chrcount cells a row (LINESTEP 2 * chrcount) */
void rrb_setup(unsigned int chrcount);

/* 8-bit characters again, chrcount cells (LINESTEP chrcount) */
void rrb_done(unsigned int chrcount);

/* DBLRR on (1) / off (0) */
void rrb_double(unsigned char on);

/* The 28-bit address of cell (row, col): its screen bytes ($D060 +
 * row * LINESTEP + col * 2) / its colour RAM bytes ($FF80000 + $D064 + the
 * same) */
unsigned long rrb_screen_cell(unsigned int row, unsigned int col);
unsigned long rrb_colour_cell(unsigned int row, unsigned int col);

/* Cell (row, col) := a GOTOX token to x (0-1023), with RRB_TRANSPARENT /
 * RRB_ROWMASK (mask: the row mask) */
void rrb_gotox(unsigned int row, unsigned int col, unsigned int x, unsigned char flags, unsigned char mask);

/* Cell (row, col) := character ch (0-8191) in colour (colour byte 1) */
void rrb_putchar(unsigned int row, unsigned int col, unsigned int ch, unsigned char colour);
