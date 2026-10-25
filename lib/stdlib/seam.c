/* seam.c — VIC-IV screens of two-byte cells (seam.h)
 *
 * The cells are written by DMA with a step of two (dma.h): a run of one
 * byte's values (the characters' low bytes, the colours) to every other
 * byte, a fill to the others.
 */
#include <seam.h>
#include <dma.h>

#define REG(a) (*(volatile unsigned char *)(a))
#define COLOUR_RAM 0xFF80000UL

/* the registers seam_show changes, in the order they are saved */
static const unsigned char seam_regs[10] = {0x54, 0x58, 0x59, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65};

unsigned long seam_cell(const seam_screen_t *s, unsigned char x, unsigned char y) {
    return s->screen + (((unsigned long)y * s->cols + x) << 1);
}

unsigned long seam_colour(const seam_screen_t *s, unsigned char x, unsigned char y) {
    return COLOUR_RAM + s->colour + (((unsigned long)y * s->cols + x) << 1);
}

void seam_clear(const seam_screen_t *s, unsigned char colour) {
    unsigned int n = (unsigned int)s->cols * s->rows;
    unsigned long c = COLOUR_RAM + s->colour;
    dma_fill_far_step(s->screen, 2, n, 0x20);
    dma_fill_far_step(s->screen + 1, 2, n, 0);
    dma_fill_far_step(c, 2, n, 0);
    dma_fill_far_step(c + 1, 2, n, colour);
}

void seam_init(seam_screen_t *s, unsigned long screen, unsigned int colour,
               unsigned char cols, unsigned char rows) {
    s->screen = screen;
    s->colour = colour;
    s->cols = cols;
    s->rows = rows;
    s->shown = 0;
    seam_clear(s, 1);
}

void seam_show(seam_screen_t *s) {
    unsigned char i;
    if (s->shown) return;
    for (i = 0; i < 10; i++) s->saved[i] = REG(0xD000 + seam_regs[i]);
    REG(0xD060) = (unsigned char)s->screen;
    REG(0xD061) = (unsigned char)(s->screen >> 8);
    REG(0xD062) = (unsigned char)(s->screen >> 16);
    REG(0xD063) = (unsigned char)((REG(0xD063) & 0xF0) | ((unsigned char)(s->screen >> 24) & 0x0F));
    REG(0xD064) = (unsigned char)s->colour;
    REG(0xD065) = (unsigned char)(s->colour >> 8);
    REG(0xD058) = (unsigned char)(s->cols << 1);
    REG(0xD059) = (unsigned char)(s->cols >> 7);
    REG(0xD05E) = s->cols;
    REG(0xD054) = REG(0xD054) | 0x05;      /* CHR16, FCLRHI */
    s->shown = 1;
}

void seam_hide(seam_screen_t *s) {
    unsigned char i;
    if (!s->shown) return;
    REG(0xD054) = s->saved[0];             /* (first: one byte a cell again) */
    for (i = 1; i < 10; i++) REG(0xD000 + seam_regs[i]) = s->saved[i];
    s->shown = 0;
}

void seam_put(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned int ch, unsigned char colour) {
    unsigned char cell[2];
    cell[0] = (unsigned char)ch;
    cell[1] = (unsigned char)(ch >> 8);
    dma_copy_far(seam_cell(s, x, y), (unsigned long)(unsigned int)cell, 2);
    cell[0] = 0;
    cell[1] = colour;
    dma_copy_far(seam_colour(s, x, y), (unsigned long)(unsigned int)cell, 2);
}

void seam_text(const seam_screen_t *s, unsigned char x, unsigned char y, const void *codes,
               unsigned char n, unsigned char colour) {
    unsigned long a = seam_cell(s, x, y), c = seam_colour(s, x, y);
    dma_copy_far_step(a, 2, (unsigned long)(unsigned int)codes, 1, n);
    dma_fill_far_step(a + 1, 2, n, 0);
    dma_fill_far_step(c, 2, n, 0);
    dma_fill_far_step(c + 1, 2, n, colour);
}

void seam_row(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned char n,
              const void *codes, const void *colours) {
    unsigned long a = seam_cell(s, x, y), c = seam_colour(s, x, y);
    dma_copy_far_step(a, 2, (unsigned long)(unsigned int)codes, 1, n);
    dma_fill_far_step(a + 1, 2, n, 0);
    dma_fill_far_step(c, 2, n, 0);
    dma_copy_far_step(c + 1, 2, (unsigned long)(unsigned int)colours, 1, n);
}

void seam_cells(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned long numbers,
                unsigned int n) {
    dma_copy_far(seam_cell(s, x, y), numbers, n << 1);
}
