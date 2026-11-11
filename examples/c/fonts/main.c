/* fonts — font45 and <font.h> (#466): four kinds of font on one screen
 *
 * The Makefile has font45 convert one TrueType font (Russo One, SIL Open
 * Font License: OFL.txt) three ways and make a segment display's font:
 *
 *   title.c  a glyph set: each glyph pre-drawn into full-colour characters
 *            (font45 --fcm), two colour ramps down it (--gradient 2) and a
 *            drop shadow; put on the screen by font_put_cells
 *   body.c   a proportional 2 bpp font (anti-aliased): font_draw_fcm draws
 *            the paragraph into a block of full-colour characters
 *   score.c  a 16-segment display's digits, unlit segments dim (--segments
 *            16 --fcm): the counter, put again each frame
 *   term.c   an 8x8 charset (--charset screencode): conio's text below in it
 *            (font_install, font_select)
 *
 * The screen is conio's full-colour mode (fcm_enable): two bytes a cell,
 * characters from 256 full-colour at their number * 64. The glyph sets are
 * copied to bank 4 (far_malloc, 64-byte aligned) so their characters are
 * numbered from 256 on, where the VIC-IV shows them as full-colour ones.
 *
 * Any key quits. FONTS_CHECK builds a test that runs 50 frames, reads the
 * screen back and reports (src/test/test_examples.sh).
 */
#include <conio.h>
#include <dma.h>
#include <far_ptr.h>
#include <font.h>
#include <palette.h>

#include "body.h"
#include "score.h"
#include "term.h"
#include "title.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define LIN(p) ((unsigned long)(unsigned int)(p))
#define ROW 160                 /* a screen row's bytes: 80 cells, two bytes each */

#define BODY_COLS 40            /* the paragraph's block of characters */
#define BODY_ROWS 5

static const char *const lines[] = {
    "A TrueType font at 11 pixels, two bits a pixel:",
    "proportional and anti-aliased, drawn by",
    "font_draw_fcm into full-colour characters.",
};

/* a copy of a font45 array in bank 4, 64-byte aligned, at *at; *at moves past it */
static unsigned long place(const unsigned char *data, unsigned int size, unsigned long *at) {
    unsigned long a = (*at + 63) & ~63UL;
    dma_copy_far(a, LIN(data), size);
    *at = a + size;
    return a;
}

/* n as six digits */
static void digits6(char *d, unsigned int n) {
    signed char i;
    for (i = 5; i >= 0; i--) {
        d[i] = (char)('0' + n % 10);
        n /= 10;
    }
    d[6] = 0;
}

static void wait_frame(void) {
    while (REG(0xD012) != 250) {}
    while (REG(0xD012) == 250) {}
}

int main(void) {
    font_t ft, fb, fs;
    far_ptr_t mem;
    unsigned long at, screen, block, charset;
    unsigned int n0, frames = 0;
    unsigned char cx, cy, i;
    char digits[8];

    /* the colours: the title's two ramps (32-34 light, 35-37 deeper) and
     * shadow (38), the paragraph's grey ramp (40-42), the segments (48 dim,
     * 50 lit) */
    palette_set_color(32, 120, 200, 255); palette_set_color(33, 170, 225, 255); palette_set_color(34, 230, 245, 255);
    palette_set_color(35, 40, 110, 220);  palette_set_color(36, 80, 150, 240);  palette_set_color(37, 140, 190, 255);
    palette_set_color(38, 20, 30, 60);
    palette_set_color(40, 90, 90, 110);   palette_set_color(41, 170, 170, 190); palette_set_color(42, 255, 255, 255);
    palette_set_color(48, 70, 10, 10);    palette_set_color(49, 150, 20, 20);   palette_set_color(50, 255, 60, 40);

    bordercolor(0);
    bgcolor(0);
    clrscr();
    /* conio's text in the 8x8 charset made from the same font */
    mem = far_malloc(40000u, BANK4);
    if (far_is_null(mem)) mem = far_malloc(40000u, BANK5);
    if (far_is_null(mem) || !fcm_enable()) {
        cputs("no room in banks 4 and 5\r\n");
        return 1;
    }
    at = far_to_linear(mem);
    charset = (at + 2047) & ~2047UL;
    font_install(LIN(term), sizeof term, charset);
    font_select(charset);
    at = charset + sizeof term;

    if (!font_from(&ft, place(title, sizeof title, &at)) || !font_from(&fs, place(score, sizeof score, &at)) ||
        !font_from(&fb, LIN(body))) {
        fcm_disable();
        cputs("a font45 font was not read\r\n");
        return 1;
    }
    screen = (unsigned long)REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16);

    /* the title: its glyphs' characters */
    font_put_cells(&ft, screen, ROW, 3, 1, "fonts");
    cputsxy(30, 2, "font45: TrueType, BDF, PSF, images and charsets");
    cputsxy(30, 3, "for the MEGA65; <font.h> draws them");

    /* the paragraph: drawn into a block of characters numbered down its columns */
    block = (at + 63) & ~63UL;
    at = block + (unsigned long)BODY_COLS * BODY_ROWS * 64;
    dma_fill_far(block, BODY_COLS * BODY_ROWS * 64, 0);
    font_ramp(40, 3);
    for (i = 0; i < 3; i++) font_draw_fcm(&fb, block, BODY_ROWS, 2, 2 + i * 12, lines[i]);
    n0 = (unsigned int)(block >> 6);
    for (cx = 0; cx < BODY_COLS; cx++)
        for (cy = 0; cy < BODY_ROWS; cy++) fcm_putglyph(3 + cx, 7 + cy, n0 + cx * BODY_ROWS + cy, 1);

    cputsxy(3, 14, "a 16-segment display's digits (font45 --segments 16 --fcm):");
    cputsxy(3, 20, "THIS TEXT IS CONIO'S, IN AN 8X8 CHARSET FONT45 MADE FROM THE SAME");
    cputsxy(3, 21, "TRUETYPE FONT AT 8 PIXELS (--CHARSET SCREENCODE).");
    cputsxy(3, 23, "ANY KEY QUITS.");

    for (;;) {
        digits6(digits, frames);
        font_put_cells(&fs, screen, ROW, 3, 16, digits);
        wait_frame();
        frames++;
#ifdef FONTS_CHECK
        if (frames == 50) break;
#else
        if (kbhit()) { cgetc(); break; }
#endif
    }
#ifdef FONTS_CHECK
    {
        /* the title's first cell is f's first character; the counter's last digit 9 */
        unsigned int t = *(unsigned char __far *)(screen + ROW + 6) | (*(unsigned char __far *)(screen + ROW + 7) << 8);
        unsigned int want = (unsigned int)(ft.glyphs >> 6);
        unsigned long nine = fs.glyphs + 9UL * fs.cols * fs.rows * 64;
        unsigned char lit = 0;
        unsigned int k;
        for (k = 0; k < 576; k++) if (*(unsigned char __far *)(nine + k) == 50) lit = 1;
        fcm_disable();
        /* (cputs, not printf: the program is near the end of bank 0's RAM) */
        clrscr();
        digits6(digits, frames);
        if (t == want && fb.bpp == 2 && ft.cols * ft.rows > 1 && lit) {
            cputs("fonts check ok, ");
            cputs(digits);
            cputs(" frames\r\n");
        } else {
            cputs("fonts check failed\r\n");
        }
        return 0;
    }
#else
    fcm_disable();
    font_select(0x29800UL);
    clrscr();
    return 0;
#endif
}
