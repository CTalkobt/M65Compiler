/* blue.c — overlay "blue" */
#pragma cc45 overlay(blue)
#include "mapdemo.h"

__naked static unsigned char *here(void) {
    __asm__("lda #<blue_mark");
    __asm__("ldx #>blue_mark");
    __asm__("rts");
    __asm__("blue_mark: .byte $42");      /* 'B' */
}

void blue_where(struct seen *s) { look(s, (unsigned int)here()); }
