/* vic4_colors — set border and screen colours through a register struct
 *
 * Overlays a volatile struct on the border and background colour registers
 * ($D020/$D021, VIC-II registers the VIC-IV keeps), steps both through a
 * few colours, then leaves a white screen with black text.
 *
 * Demonstrates: volatile struct pointer overlay on hardware I/O.
 * (crt0 has already enabled the VIC-IV registers: $47 $53 to $D02F.)
 */

#include <stdio.h>

/* Border and background colour registers at $D020 */
typedef struct {
    unsigned char border;      /* $D020: border colour */
    unsigned char screen;      /* $D021: screen (background) colour */
} vic_colors_t;

volatile vic_colors_t *VIC = (vic_colors_t *)0xD020;

/* The raster line ($D012, bit 8 in $D011 bit 7) */
#define RASTER    (*(volatile unsigned char *)0xD012)
#define RASTER_HI (*(volatile unsigned char *)0xD011)

/* Wait for n frames: leave raster line 0, then wait for it again */
static void wait_frames(unsigned char n) {
    while (n--) {
        while (RASTER == 0) ;
        while (RASTER != 0 || (RASTER_HI & 0x80)) ;
    }
}

int main(void) {
    static const unsigned char colors[] = {2, 5, 6, 7, 14, 0};  /* red green blue yellow lblue black */
    unsigned char i;

    /* Step border and background through the colours, half a second each */
    for (i = 0; i < sizeof colors; i++) {
        VIC->border = colors[i];
        VIC->screen = colors[(i + 3) % sizeof colors];
        wait_frames(25);
    }

    /* End on white, with black text (PETSCII $90) so it stays readable */
    VIC->border = 1;
    VIC->screen = 1;
    putchar(0x90);
    puts("done.");
    return 0;
}
