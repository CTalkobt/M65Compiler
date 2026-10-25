/* palette.c — the VIC-IV's colour palette (palette.h)
 *
 * The registers hold each component with its nibbles swapped (red at
 * $D100 + n, green $D200 + n, blue $D300 + n).
 */
#include <palette.h>

#define PAL(c, n) (*(volatile unsigned char *)(0xD100 + ((unsigned int)(c) << 8) + (unsigned char)(n)))

static unsigned char swap(unsigned char v) {
    return (unsigned char)(v << 4 | v >> 4);
}

void palette_set_color(int index, uint8_t r, uint8_t g, uint8_t b) {
    if ((unsigned int)index > 255) return;
    PAL(0, index) = swap(r);
    PAL(1, index) = swap(g);
    PAL(2, index) = swap(b);
}

void palette_set_color_rgb(int index, rgb_color_t color) {
    palette_set_color(index, color.r, color.g, color.b);
}

int palette_get_color(int index, rgb_color_t *out) {
    if ((unsigned int)index > 255) return -1;
    out->r = swap(PAL(0, index));
    out->g = swap(PAL(1, index));
    out->b = swap(PAL(2, index));
    return 0;
}

void palette_set_range(int first, int count, const uint8_t *rgb) {
    while (count-- > 0) {
        palette_set_color(first++, rgb[0], rgb[1], rgb[2]);
        rgb += 3;
    }
}

void palette_save(int first, int count, uint8_t *buf) {
    unsigned char c;
    int i;
    for (c = 0; c < 3; c++)
        for (i = 0; i < count; i++) *buf++ = PAL(c, first + i);
}

void palette_restore(int first, int count, const uint8_t *buf) {
    unsigned char c;
    int i;
    for (c = 0; c < 3; c++)
        for (i = 0; i < count; i++) PAL(c, first + i) = *buf++;
}

void palette_rotate(int start, int end, int direction) {
    unsigned char c, keep;
    int i;
    if (start >= end || (unsigned int)start > 255 || (unsigned int)end > 255) return;
    for (c = 0; c < 3; c++) {
        if (direction > 0) {
            keep = PAL(c, end);
            for (i = end; i > start; i--) PAL(c, i) = PAL(c, i - 1);
            PAL(c, start) = keep;
        } else if (direction < 0) {
            keep = PAL(c, start);
            for (i = start; i < end; i++) PAL(c, i) = PAL(c, i + 1);
            PAL(c, end) = keep;
        }
    }
}
