/* red.c — overlay "red": every byte of code in this file lives in the
 * overlay file, kept in bank 5, and runs from there through the window.
 */
#pragma cc45 overlay(red)
#include "mapdemo.h"

/* The address of a byte of this overlay's own code: `mark`, a byte placed
 * right after the function's rts. ln45 linked this overlay to run in the
 * window, so this is a window address ($6xxx), the same for whichever
 * overlay is mapped there. (A label that is not a function's name: taking a
 * function's address would give its resident stub instead.) */
__naked static unsigned char *here(void) {
    __asm__("lda #<red_mark");
    __asm__("ldx #>red_mark");
    __asm__("rts");
    __asm__("red_mark: .byte $52");       /* 'R' */
}

void red_where(struct seen *s) { look(s, (unsigned int)here()); }

/* Code is RAM: a store to the window lands in bank 5 -- if the window is
 * mapped there, and not a copy */
void red_poke(unsigned char v) { *here() = v; }

unsigned char red_peek(void) { return *here(); }
