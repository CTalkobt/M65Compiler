/* palette.h — the VIC-IV's colour palette (lib/stdlib/palette.c)
 *
 * The palette has 256 entries ($D100-$D3FF: red, green, blue), each
 * component 8 bits with its nibbles swapped in the register (the C65's
 * 4-bit values in the low nibble); these functions take and return plain
 * 0-255 values. Entries 0-15 are the text colours; full-colour characters
 * and other VIC-IV modes use all 256. They work on the palette bank mapped
 * at $D100 ($D070's MAPEDPAL; bank 0 at reset, the one shown).
 *
 * Usage:
 *   #include <palette.h>
 *   palette_set_color(48, 255, 136, 200);          // entry 48: pink
 *   static unsigned char saved[3 * 16];
 *   palette_save(0, 16, saved);                    // ... and later
 *   palette_restore(0, 16, saved);
 */

#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

typedef struct {
    uint8_t r;  /* Red component (0-255) */
    uint8_t g;  /* Green component (0-255) */
    uint8_t b;  /* Blue component (0-255) */
} rgb_color_t;

/* the text colours' entries */
#define PAL_BLACK       0
#define PAL_WHITE       1
#define PAL_RED         2
#define PAL_CYAN        3
#define PAL_MAGENTA     4
#define PAL_GREEN       5
#define PAL_BLUE        6
#define PAL_YELLOW      7
#define PAL_ORANGE      8
#define PAL_BROWN       9
#define PAL_LIGHT_RED   10
#define PAL_DARK_GRAY   11
#define PAL_MEDIUM_GRAY 12
#define PAL_LIGHT_GREEN 13
#define PAL_LIGHT_BLUE  14
#define PAL_LIGHT_GRAY  15

/* Set entry index (0-255) to r, g, b (0-255 each) */
void palette_set_color(int index, uint8_t r, uint8_t g, uint8_t b);
void palette_set_color_rgb(int index, rgb_color_t color);

/* Entry index's colour into *out; 0 (-1 for an index out of 0-255) */
int palette_get_color(int index, rgb_color_t *out);

/* Set count entries from first: rgb holds r, g, b for each (3 x count bytes) */
void palette_set_range(int first, int count, const uint8_t *rgb);

/* Copy count entries from first into buf (3 x count bytes: the registers as
 * they are, all the reds, the greens, the blues) and put them back */
void palette_save(int first, int count, uint8_t *buf);
void palette_restore(int first, int count, const uint8_t *buf);

/* Rotate entries start..end by one: direction > 0 moves each colour to the
 * next entry (end's to start), < 0 the other way (colour cycling) */
void palette_rotate(int start, int end, int direction);

#endif /* PALETTE_H */
