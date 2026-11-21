/* test_palette_xemu.c — <palette.h> (stdlib_asm/palette.s45) at its edges
 *
 * Results at $C0C0 (01 = pass):
 *   [0] palette_set_color / palette_set_color_rgb / palette_get_color at
 *       entries 0 and 255; an index out of 0-255 (256, -1) changes nothing
 *       and get gives -1
 *   [1] palette_set_range: entries past 255 and below 0 skipped; a count of
 *       0 or less does nothing
 *   [2] palette_save / palette_restore: the whole palette (count 256: all
 *       the reds, the greens, the blues, the registers as they are); the
 *       entry wraps at 256; a count of 0 or less writes nothing
 *   [3] palette_rotate both ways over 0-255 and two entries; nothing for
 *       start >= end, an entry past 255 or a direction of 0; any positive /
 *       negative direction is one step
 *   [4] 01
 *   [5] $AA sentinel
 */
#include <palette.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char saved[768];
static unsigned char work[768];

static unsigned char swap(unsigned char v) {
    return (unsigned char)(v << 4 | v >> 4);
}

static int is(int i, unsigned char r, unsigned char g, unsigned char b) {
    rgb_color_t c;
    if (palette_get_color(i, &c) != 0) return 0;
    return c.r == r && c.g == g && c.b == b;
}

static int test_set_get(void) {
    rgb_color_t k = {0x9C, 0x05, 0xE7};
    rgb_color_t c;
    palette_set_color(0, 0x12, 0x34, 0x56);
    palette_set_color(255, 0xAB, 0xCD, 0xEF);
    if (REG(0xD100) != 0x21 || REG(0xD200) != 0x43 || REG(0xD300) != 0x65) return 0;
    if (REG(0xD1FF) != 0xBA || REG(0xD2FF) != 0xDC || REG(0xD3FF) != 0xFE) return 0;
    if (!is(0, 0x12, 0x34, 0x56) || !is(255, 0xAB, 0xCD, 0xEF)) return 0;
    palette_set_color(256, 1, 2, 3);          /* would be entry 0 */
    palette_set_color(-1, 1, 2, 3);           /* would be entry 255 */
    palette_set_color_rgb(256, k);
    if (!is(0, 0x12, 0x34, 0x56) || !is(255, 0xAB, 0xCD, 0xEF)) return 0;
    palette_set_color_rgb(17, k);
    if (REG(0xD100 + 17) != 0xC9 || REG(0xD200 + 17) != 0x50 || REG(0xD300 + 17) != 0x7E) return 0;
    if (!is(17, 0x9C, 0x05, 0xE7)) return 0;
    c.r = c.g = c.b = 0x77;
    if (palette_get_color(256, &c) != -1 || palette_get_color(-1, &c) != -1) return 0;
    return c.r == 0x77 && c.g == 0x77 && c.b == 0x77;
}

static int test_range(void) {
    static const unsigned char rgb[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    palette_set_color(0, 0, 0, 0);
    palette_set_color(1, 0, 0, 0);
    palette_set_range(254, 4, rgb);           /* 254, 255; 256 and 257 skipped */
    if (!is(254, 1, 2, 3) || !is(255, 4, 5, 6) || !is(0, 0, 0, 0) || !is(1, 0, 0, 0)) return 0;
    palette_set_range(-1, 2, rgb);            /* -1 skipped, 0 from rgb[3] */
    if (!is(0, 4, 5, 6) || !is(255, 4, 5, 6)) return 0;
    palette_set_range(40, 0, rgb);
    palette_set_range(40, -3, rgb);
    palette_set_color(40, 9, 9, 9);
    palette_set_range(40, 0, rgb);
    palette_set_range(40, -300, rgb);
    return is(40, 9, 9, 9);
}

static int test_save(void) {
    unsigned int i;
    for (i = 0; i < 256; i++) palette_set_color(i, i, i ^ 0x5A, 255 - i);
    palette_save(0, 256, saved);
    for (i = 0; i < 256; i++)
        if (saved[i] != swap(i) || saved[256 + i] != swap(i ^ 0x5A) || saved[512 + i] != swap(255 - i)) return 0;
    for (i = 0; i < 256; i++) palette_set_color(i, 0, 0, 0);
    palette_restore(0, 256, saved);
    for (i = 0; i < 256; i += 17)
        if (!is(i, i, i ^ 0x5A, 255 - i)) return 0;
    if (!is(255, 255, 255 ^ 0x5A, 0)) return 0;
    /* the entry wraps: 255 then 0 */
    palette_save(255, 2, work);
    if (work[0] != swap(255) || work[1] != swap(0) || work[2] != swap(255 ^ 0x5A) || work[3] != swap(0x5A) ||
        work[4] != swap(0) || work[5] != swap(255))
        return 0;
    work[0] = swap(0x31);
    work[1] = swap(0x32);
    palette_restore(255, 2, work);
    if (!is(255, 0x31, 255 ^ 0x5A, 0) || !is(0, 0x32, 0x5A, 255)) return 0;
    /* a count of 0 or less writes nothing */
    work[0] = 0xEE;
    palette_save(3, 0, work);
    palette_save(3, -1, work);
    palette_restore(3, 0, work);
    palette_restore(3, -1, work);
    return work[0] == 0xEE && is(3, 3, 3 ^ 0x5A, 252);
}

static int test_rotate(void) {
    unsigned int i;
    for (i = 0; i < 256; i++) palette_set_color(i, i, 255 - i, i ^ 0x33);
    palette_rotate(0, 255, 1);                /* entry i has i - 1's; 0 has 255's */
    if (!is(0, 255, 0, 255 ^ 0x33) || !is(1, 0, 255, 0x33) || !is(255, 254, 1, 254 ^ 0x33)) return 0;
    palette_rotate(0, 255, -7);               /* back */
    for (i = 0; i < 256; i += 15)
        if (!is(i, i, 255 - i, i ^ 0x33)) return 0;
    if (!is(255, 255, 0, 255 ^ 0x33)) return 0;
    palette_rotate(0, 255, -1);               /* entry i has i + 1's; 255 has 0's */
    if (!is(0, 1, 254, 1 ^ 0x33) || !is(254, 255, 0, 255 ^ 0x33) || !is(255, 0, 255, 0x33)) return 0;
    palette_rotate(0, 255, 1000);
    if (!is(0, 0, 255, 0x33) || !is(255, 255, 0, 255 ^ 0x33)) return 0;
    palette_rotate(100, 101, 1);              /* two entries: swapped */
    if (!is(100, 101, 154, 101 ^ 0x33) || !is(101, 100, 155, 100 ^ 0x33)) return 0;
    palette_rotate(100, 101, -1);
    if (!is(100, 100, 155, 100 ^ 0x33) || !is(99, 99, 156, 99 ^ 0x33) || !is(102, 102, 153, 102 ^ 0x33)) return 0;
    /* nothing */
    palette_rotate(50, 50, 1);
    palette_rotate(60, 50, 1);
    palette_rotate(50, 256, 1);
    palette_rotate(-1, 50, 1);
    palette_rotate(50, 60, 0);
    for (i = 48; i < 62; i++)
        if (!is(i, i, 255 - i, i ^ 0x33)) return 0;
    return 1;
}

int main(void) {
    OUT[0] = test_set_get();
    OUT[1] = test_range();
    OUT[2] = test_save();
    OUT[3] = test_rotate();
    OUT[4] = 1;
    OUT[5] = 0xAA;
    return 0;
}
