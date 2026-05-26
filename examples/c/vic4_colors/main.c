/* vic4_colors — Set screen/border colors via VIC-IV register struct
 *
 * Maps VIC-IV registers to I/O space using the MEGA65 I/O unlock
 * sequence, then sets border and screen (background) color to white
 * through a mapped VIC4 struct.
 *
 * Demonstrates: volatile struct pointer overlay on hardware I/O,
 * inline assembly for MEGA65 I/O unlock.
 */

#include <stdio.h>

/* VIC-IV color/display registers mapped at $D020+ */
typedef struct {
    unsigned char border;      /* $D020: border color */
    unsigned char screen;      /* $D021: screen (background) color */
} vic4_colors_t;

volatile vic4_colors_t *VIC4 = (vic4_colors_t *)0xD020;

static void unlock_mega65_io() {
    asm("lda #$47");
    asm("sta $d02f");
    asm("lda #$53");
    asm("sta $d02f");
}

int main() {
    unlock_mega65_io();

    /* Set border and screen color to white (1) */
    VIC4->border = 1;
    VIC4->screen = 1;

    puts("done.");
    return 0;
}
