/* test_seam_xemu.c — dma.h's stepped DMA, palette.h, seam.h (two-byte
 * cell screens with full-colour characters)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] dma_copy_far_step / dma_fill_far_step: every other byte written,
 *       the others kept; a source step of 2 gathers every other byte
 *   [1] palette_set_color / palette_get_color (the registers' nibbles
 *       swapped), palette_save / palette_restore, palette_set_range
 *   [2] palette_rotate both ways
 *   [3] seam_init clears to spaces in colour 1; seam_text, seam_put,
 *       seam_row write the cells' two screen bytes and two colour bytes
 *   [4] seam_cells copies character numbers from far memory; seam_cell /
 *       seam_colour give the addresses
 *   [5] seam_show: CHR16 + FCLRHI, the stride, the pointers; seam_hide
 *       puts back every register
 *   [6] 01
 *   [7] $AA sentinel
 */
#include <dma.h>
#include <palette.h>
#include <seam.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))
#define FAR 0x58000UL
#define COLOUR_RAM 0xFF80000UL

static unsigned char buf[64];
static seam_screen_t scr;

static void get(unsigned long lin, unsigned int n) {
    dma_copy_far((unsigned long)(unsigned int)buf, lin, n);
}

static int test_dma_step(void) {
    unsigned char i;
    static unsigned char src[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    dma_fill_far(FAR, 16, 0xEE);
    dma_copy_far_step(FAR, 2, (unsigned long)(unsigned int)src, 1, 4);
    dma_fill_far_step(FAR + 9, 2, 3, 0x55);
    get(FAR, 16);
    for (i = 0; i < 4; i++)
        if (buf[2 * i] != i + 1 || buf[2 * i + 1] != 0xEE) return 0;
    if (buf[8] != 0xEE || buf[9] != 0x55 || buf[10] != 0xEE || buf[11] != 0x55 || buf[13] != 0x55 || buf[14] != 0xEE ||
        buf[15] != 0xEE)
        return 0;
    /* gather every other byte: 1, 3, 5, 7 */
    dma_copy_far_step((unsigned long)(unsigned int)buf, 1, (unsigned long)(unsigned int)src, 2, 4);
    return buf[0] == 1 && buf[1] == 3 && buf[2] == 5 && buf[3] == 7;
}

static int test_palette(void) {
    rgb_color_t c;
    static unsigned char saved[12];
    static const unsigned char rgb[6] = {10, 20, 30, 0xF1, 0x2E, 0x80};
    palette_set_color(200, 0x12, 0xAB, 0xF0);
    if (REG(0xD100 + 200) != 0x21 || REG(0xD200 + 200) != 0xBA || REG(0xD300 + 200) != 0x0F) return 0;
    if (palette_get_color(200, &c) != 0 || c.r != 0x12 || c.g != 0xAB || c.b != 0xF0) return 0;
    if (palette_get_color(256, &c) != -1) return 0;
    palette_save(200, 4, saved);
    palette_set_range(200, 2, rgb);
    palette_get_color(201, &c);
    if (c.r != 0xF1 || c.g != 0x2E || c.b != 0x80) return 0;
    palette_restore(200, 4, saved);
    palette_get_color(200, &c);
    return c.r == 0x12 && c.g == 0xAB && c.b == 0xF0;
}

static int test_rotate(void) {
    rgb_color_t c;
    palette_set_color(210, 1, 1, 1);
    palette_set_color(211, 2, 2, 2);
    palette_set_color(212, 3, 3, 3);
    palette_rotate(210, 212, 1);           /* 3 1 2 */
    palette_get_color(210, &c);
    if (c.r != 3) return 0;
    palette_get_color(212, &c);
    if (c.g != 2) return 0;
    palette_rotate(210, 212, -1);          /* back: 1 2 3 */
    palette_get_color(210, &c);
    if (c.b != 1) return 0;
    palette_get_color(212, &c);
    return c.r == 3;
}

static int test_write(void) {
    static const unsigned char codes[3] = {1, 2, 3};
    static const unsigned char cols[3] = {7, 8, 9};
    seam_init(&scr, FAR + 0x100, 0x2000, 40, 10);
    get(scr.screen + 2 * 39, 4);           /* (a row's last cell, the next row's first) */
    if (buf[0] != 0x20 || buf[1] != 0 || buf[2] != 0x20 || buf[3] != 0) return 0;
    get(COLOUR_RAM + 0x2000 + 2 * 399, 2);
    if (buf[0] != 0 || buf[1] != 1) return 0;
    seam_text(&scr, 5, 2, codes, 3, 4);
    get(seam_cell(&scr, 5, 2), 6);
    if (buf[0] != 1 || buf[1] != 0 || buf[2] != 2 || buf[4] != 3 || buf[5] != 0) return 0;
    get(seam_colour(&scr, 5, 2), 6);
    if (buf[0] != 0 || buf[1] != 4 || buf[3] != 4 || buf[5] != 4) return 0;
    seam_put(&scr, 39, 9, 0x1234, 5);
    get(scr.screen + 2 * 399, 2);
    if (buf[0] != 0x34 || buf[1] != 0x12) return 0;
    get(COLOUR_RAM + 0x2000 + 2 * 399, 2);
    if (buf[0] != 0 || buf[1] != 5) return 0;
    seam_row(&scr, 0, 1, 3, codes, cols);
    get(seam_colour(&scr, 0, 1), 6);
    return buf[1] == 7 && buf[3] == 8 && buf[5] == 9;
}

static int test_cells(void) {
    static const unsigned char nums[6] = {0x00, 0x10, 0x01, 0x10, 0x02, 0x10};
    dma_copy_far(FAR + 0x2000, (unsigned long)(unsigned int)nums, 6);
    seam_cells(&scr, 38, 3, FAR + 0x2000, 3);       /* (to the next row's first cell) */
    get(seam_cell(&scr, 38, 3), 6);
    if (buf[0] != 0x00 || buf[1] != 0x10 || buf[3] != 0x10 || buf[4] != 0x02) return 0;
    if (seam_cell(&scr, 0, 4) != seam_cell(&scr, 38, 3) + 4) return 0;
    return seam_colour(&scr, 1, 0) == COLOUR_RAM + 0x2002;
}

static int test_show(void) {
    static unsigned char before[10];
    static const unsigned char regs[10] = {0x54, 0x58, 0x59, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65};
    unsigned char i;
    for (i = 0; i < 10; i++) before[i] = REG(0xD000 + regs[i]);
    seam_show(&scr);
    if ((REG(0xD054) & 0x05) != 0x05 || REG(0xD058) != 80 || REG(0xD059) != 0 || REG(0xD05E) != 40) return 0;
    if (REG(0xD060) != (unsigned char)scr.screen || REG(0xD061) != (unsigned char)(scr.screen >> 8) ||
        REG(0xD062) != 5 || (REG(0xD063) & 0x0F) != 0 || REG(0xD064) != 0x00 || REG(0xD065) != 0x20)
        return 0;
    seam_hide(&scr);
    for (i = 0; i < 10; i++)
        if (REG(0xD000 + regs[i]) != before[i]) return 0;
    return 1;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    OUT[0] = test_dma_step() ? 1 : 0;
    OUT[1] = test_palette() ? 1 : 0;
    OUT[2] = test_rotate() ? 1 : 0;
    OUT[3] = test_write() ? 1 : 0;
    OUT[4] = test_cells() ? 1 : 0;
    OUT[5] = test_show() ? 1 : 0;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
