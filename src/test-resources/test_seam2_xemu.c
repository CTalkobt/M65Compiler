/* test_seam2_xemu.c — seam.h's edges (lib/stdlib_asm/seam.s45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] seam_cell / seam_colour: offsets over 16 bits ((y * cols + x) * 2),
 *       a screen address with bits 24-27
 *   [1] seam_put at an odd address whose cell crosses a 64 KB boundary
 *       (screen and colour RAM); the bytes around it kept
 *   [2] seam_text and seam_row across that boundary; n = 0 writes nothing
 *   [3] seam_clear in a colour: every cell, nothing after the last
 *   [4] seam_show of 200 columns ($D059 = 1), the screen's bits 24-27 in
 *       $D063 with its high nibble kept; shown twice and hidden twice
 *   [5] seam_init sets the fields (shown 0) and clears in colour 1
 *   [6] 01
 *   [7] $AA sentinel
 */
#include <dma.h>
#include <seam.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))
#define COLOUR_RAM 0xFF80000UL

static unsigned char buf[16];
static seam_screen_t s;

static void get(unsigned long lin, unsigned int n) {
    dma_copy_far((unsigned long)(unsigned int)buf, lin, n);
}

static void set(seam_screen_t *p, unsigned long screen, unsigned int colour, unsigned char cols, unsigned char rows) {
    p->screen = screen;
    p->colour = colour;
    p->cols = cols;
    p->rows = rows;
    p->shown = 0;
}

static int test_addr(void) {
    set(&s, 0x40001UL, 0x0001, 250, 200);
    if (seam_cell(&s, 249, 199) != 0x40001UL + 99998UL) return 0;
    if (seam_colour(&s, 249, 199) != COLOUR_RAM + 1 + 99998UL) return 0;
    if (seam_cell(&s, 0, 0) != 0x40001UL || seam_colour(&s, 3, 0) != COLOUR_RAM + 7) return 0;
    set(&s, 0x8000100UL, 0x7F00, 80, 25);
    if (seam_cell(&s, 1, 1) != 0x8000100UL + 162) return 0;
    return seam_colour(&s, 79, 0) == COLOUR_RAM + 0x7F00 + 158;
}

static int test_put(void) {
    dma_fill_far(0x4FFFCUL, 8, 0xEE);
    dma_fill_far(COLOUR_RAM + 0x3FFC, 8, 0xEE);
    set(&s, 0x4FFFFUL, 0x3FFF, 40, 25);
    seam_put(&s, 0, 0, 0xABCD, 7);
    get(0x4FFFCUL, 8);
    if (buf[2] != 0xEE || buf[3] != 0xCD || buf[4] != 0xAB || buf[5] != 0xEE) return 0;
    get(COLOUR_RAM + 0x3FFC, 8);
    return buf[2] == 0xEE && buf[3] == 0 && buf[4] == 7 && buf[5] == 0xEE;
}

static int test_runs(void) {
    static const unsigned char codes[3] = {0x41, 0x42, 0x43};
    static const unsigned char cols[3] = {9, 10, 11};
    unsigned char i;
    dma_fill_far(0x4FFF0UL, 16, 0xEE);
    dma_fill_far(COLOUR_RAM + 0x3FF0, 16, 0xEE);
    set(&s, 0x4FFF1UL, 0x3FF1, 40, 25);
    seam_text(&s, 5, 0, codes, 3, 2);          /* from $4FFFB: 6 bytes over $50000 */
    get(0x4FFF0UL, 16);
    if (buf[10] != 0xEE || buf[11] != 0x41 || buf[12] != 0 || buf[13] != 0x42 || buf[14] != 0 || buf[15] != 0x43)
        return 0;
    get(0x50000UL, 2);
    if (buf[0] != 0 || buf[1] != 0xEE) return 0;
    get(COLOUR_RAM + 0x3FF0, 16);
    if (buf[10] != 0xEE || buf[11] != 0 || buf[12] != 2 || buf[13] != 0 || buf[14] != 2) return 0;
    seam_row(&s, 5, 0, 3, codes, cols);
    get(COLOUR_RAM + 0x3FFB, 6);
    if (buf[1] != 9 || buf[3] != 10 || buf[5] != 11) return 0;
    for (i = 0; i < 6; i += 2)
        if (buf[i] != 0) return 0;
    seam_text(&s, 0, 0, codes, 0, 2);          /* nothing */
    seam_row(&s, 0, 0, 0, codes, cols);
    get(0x4FFF0UL, 4);
    return buf[0] == 0xEE && buf[1] == 0xEE && buf[2] == 0xEE && buf[3] == 0xEE;
}

static int test_clear(void) {
    unsigned char i;
    dma_fill_far(0x44000UL, 64, 0xEE);
    dma_fill_far(COLOUR_RAM + 0x2000, 64, 0xEE);
    set(&s, 0x44001UL, 0x2001, 5, 3);          /* 15 cells: 30 bytes */
    seam_clear(&s, 6);
    get(0x44000UL, 16);
    if (buf[0] != 0xEE || buf[1] != 0x20 || buf[2] != 0) return 0;
    get(0x44000UL + 29, 4);
    if (buf[0] != 0x20 || buf[1] != 0 || buf[2] != 0xEE) return 0;
    get(COLOUR_RAM + 0x2000, 16);
    if (buf[0] != 0xEE) return 0;
    for (i = 1; i < 15; i += 2)
        if (buf[i] != 0 || buf[i + 1] != 6) return 0;
    get(COLOUR_RAM + 0x2000 + 29, 4);
    return buf[0] == 0 && buf[1] == 6 && buf[2] == 0xEE;
}

static int test_show(void) {
    static unsigned char before[10];
    static const unsigned char regs[10] = {0x54, 0x58, 0x59, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65};
    unsigned char i, d063 = REG(0xD063);
    for (i = 0; i < 10; i++) before[i] = REG(0xD000 + regs[i]);
    set(&s, 0x3050607UL, 0x1234, 200, 2);
    seam_show(&s);
    if (!s.shown || REG(0xD058) != 0x90 || REG(0xD059) != 1 || REG(0xD05E) != 200) return 0;
    if (REG(0xD060) != 7 || REG(0xD061) != 6 || REG(0xD062) != 5 || REG(0xD063) != ((d063 & 0xF0) | 3)) return 0;
    if (REG(0xD064) != 0x34 || REG(0xD065) != 0x12 || (REG(0xD054) & 5) != 5) return 0;
    seam_show(&s);                             /* again: what was saved stays */
    for (i = 0; i < 10; i++)
        if (s.saved[i] != before[i]) return 0;
    seam_hide(&s);
    if (s.shown) return 0;
    REG(0xD05E) = 77;                          /* hidden again: nothing put back */
    seam_hide(&s);
    if (REG(0xD05E) != 77) return 0;
    REG(0xD05E) = before[3];
    for (i = 0; i < 10; i++)
        if (REG(0xD000 + regs[i]) != before[i]) return 0;
    return 1;
}

static int test_init(void) {
    static seam_screen_t t;
    t.shown = 9;
    dma_fill_far(0x46000UL, 8, 0xEE);
    seam_init(&t, 0x46002UL, 0x2800, 2, 1);
    if (t.screen != 0x46002UL || t.colour != 0x2800 || t.cols != 2 || t.rows != 1 || t.shown != 0) return 0;
    get(0x46000UL, 8);
    if (buf[1] != 0xEE || buf[2] != 0x20 || buf[3] != 0 || buf[4] != 0x20 || buf[5] != 0 || buf[6] != 0xEE) return 0;
    get(COLOUR_RAM + 0x2800, 4);
    return buf[0] == 0 && buf[1] == 1 && buf[2] == 0 && buf[3] == 1;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    OUT[0] = test_addr() ? 1 : 0;
    OUT[1] = test_put() ? 1 : 0;
    OUT[2] = test_runs() ? 1 : 0;
    OUT[3] = test_clear() ? 1 : 0;
    OUT[4] = test_show() ? 1 : 0;
    OUT[5] = test_init() ? 1 : 0;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
