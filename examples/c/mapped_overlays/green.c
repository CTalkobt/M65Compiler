/* green.c — overlay "green": calls into another overlay, blue, which must
 * take the window from green while it runs, and give it back after
 */
#pragma cc45 overlay(green)
#include "mapdemo.h"

__naked static unsigned char *here(void) {
    __asm__("lda #<green_mark");
    __asm__("ldx #>green_mark");
    __asm__("rts");
    __asm__("green_mark: .byte $47");     /* 'G' */
}

void green_where(struct seen *s) { look(s, (unsigned int)here()); }

/* green, then blue (a call through blue's stub: the window is mapped to
 * blue), then green again (blue's return goes through the runtime, which
 * maps green back before green's code goes on) */
void green_then_blue(struct seen *before, struct seen *inside, struct seen *after) {
    look(before, (unsigned int)here());
    blue_where(inside);
    look(after, (unsigned int)here());
}
