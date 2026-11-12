/* fonts — font45 and <font.h> (#466): four kinds of font on one screen, the
 * title animated
 *
 * The Makefile has font45 convert one TrueType font (Russo One, SIL Open
 * Font License: OFL.txt) three ways and make a segment display's font:
 *
 *   title.c  80 pixels, 2 bpp, proportional, the five letters of the title
 *            alone (font45 --map puts o, n, t, s after f: codes f-j). The
 *            title is drawn in layers by font_draw_bitmap (in assembly) into a
 *            linear buffer: a drop shadow, a one-pixel outline, the letters,
 *            each letter in its own palette entries, a ten-band gradient down
 *            them; then copied into a block of full-colour characters. The
 *            letters fade in one after another, then a highlight sweeps across
 *            them: the palette alone changes, no character does.
 *   body.c   11 pixels, 2 bpp, proportional (anti-aliased): the paragraph,
 *            typed into another block of characters a letter at a time
 *   score.c  a 16-segment display's digits, unlit segments dim (--segments
 *            16 --fcm, a glyph set): the counter, put by font_put_cells
 *   term.c   an 8x8 charset (--charset screencode --center): conio's text in
 *            it (font_install, font_select)
 *
 * The screen is conio's full-colour mode (fcm_enable): two bytes a cell,
 * characters from 256 full-colour at their number * 64, in bank 4
 * (far_malloc, 64-byte aligned) so their numbers are 256 and up.
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

#define TCOLS 25                /* the title's block of characters (column by column) */
#define TROWS 11
#define TPITCH (TCOLS * 8)      /* its linear buffer's row */
#define BCOLS 46                /* the paragraph's */
#define BROWS 5

/* the palette entries */
#define SEG 16                  /* the segments (font45 --palette 16,3): 16 dim, 18 lit */
#define BODY 24                 /* the paragraph's three levels */
#define SHADOW 28               /* the title's shadow (three levels, one colour) */
#define OUTLINE 32              /* its outline */
#define LETTERS 64              /* the letters: 64 + letter * 18 + band * 3 + level - 1 */

#define TITLE "fghij"           /* f, o, n, t, s: font45 --map put them at f-j */
#define NLET 5
#define BANDS 10
#define PER (BANDS * 3)         /* a letter's entries */

static const char *const lines[] = {
    "A TrueType font at 11 pixels, two bits a pixel:",
    "proportional and anti-aliased, typed a letter at",
    "a time into full-colour characters by font_draw_fcm.",
};

/* the gradient's bands, top to bottom: light cyan to sky blue to violet */
static const unsigned char grad[3][3] = {{235, 252, 255}, {105, 185, 255}, {95, 60, 210}};
static unsigned char band_rgb[BANDS][3];

static void make_gradient(void) {
    unsigned char b, k;
    for (b = 0; b < BANDS; b++) {
        /* (0..18: the first half between the first two colours, the rest the last two) */
        unsigned char t = (unsigned char)(b * 18 / (BANDS - 1)), h = t < 9 ? 0 : 1, f = t < 9 ? t : t - 9;
        for (k = 0; k < 3; k++)
            band_rgb[b][k] = (unsigned char)(((unsigned int)grad[h][k] * (9 - f) + (unsigned int)grad[h + 1][k] * f) / 9);
    }
}

/* a copy of a font45 array in bank 4, 64-byte aligned, at *at; *at moves past it */
static unsigned long place(const unsigned char *data, unsigned int size, unsigned long *at) {
    unsigned long a = (*at + 63) & ~63UL;
    dma_copy_far(a, LIN(data), size);
    *at = a + size;
    return a;
}

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

/* a block of characters (column by column) on the screen at col, row */
static void put_block(unsigned long block, unsigned char cols, unsigned char rows, unsigned char col, unsigned char row) {
    unsigned int n = (unsigned int)(block >> 6);
    unsigned char cx, cy;
    for (cx = 0; cx < cols; cx++)
        for (cy = 0; cy < rows; cy++) fcm_putglyph(col + cx, row + cy, n + cx * rows + cy, 1);
}

/* The title: drawn into a linear buffer (font_draw_bitmap, assembly) in
 * layers, the shadow, the outline (the letters at the 8 offsets round them)
 * and the letters (letter i in entries LETTERS + i * PER ...); then the
 * gradient (each letter's pixel moved to its row's band, over the rows the
 * letters cover); then copied into the block of characters, 8 bytes at a
 * time (DMA), a character's rows from the buffer's */
static void draw_title(const font_t *f, unsigned long tb, unsigned long lin) {
    static const signed char ox[8] = {-1, 0, 1, -1, 1, -1, 0, 1}, oy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    char one[2];
    unsigned char i, k, cx, cy, r, y, y0 = 255, y1 = 0, band;
    unsigned int c;
    signed int x;
    one[1] = 0;
    font_ramp(SHADOW, 3);
    font_draw_bitmap(f, lin, TPITCH, 4 + 3, 2 + 3, TITLE);
    font_ramp(OUTLINE, 3);
    for (k = 0; k < 8; k++) font_draw_bitmap(f, lin, TPITCH, 4 + ox[k], 2 + oy[k], TITLE);
    x = 4;
    for (i = 0; i < NLET; i++) {
        one[0] = TITLE[i];
        font_ramp((unsigned char)(LETTERS + i * PER), 3);
        x = font_draw_bitmap(f, lin, TPITCH, x, 2, one);
    }
    /* the rows the letters cover, then their bands */
    for (y = 0; y < TROWS * 8; y++)
        for (c = 0; c < TPITCH; c++) {
            unsigned char v = *(unsigned char __far *)(lin + (unsigned long)y * TPITCH + c);
            if (v >= LETTERS && v < LETTERS + NLET * PER) {
                if (y < y0) y0 = y;
                y1 = y;
                break;
            }
        }
    for (y = y0; y <= y1; y++) {
        unsigned char __far *p = (unsigned char __far *)(lin + (unsigned long)y * TPITCH);
        band = (unsigned char)((unsigned int)(y - y0) * BANDS / (y1 - y0 + 1));
        for (c = 0; c < TPITCH; c++) {
            unsigned char v = p[c];
            if (v >= LETTERS && v < LETTERS + NLET * PER) p[c] = (unsigned char)(v + band * 3);
        }
    }
    for (cx = 0; cx < TCOLS; cx++)
        for (cy = 0; cy < TROWS; cy++)
            for (r = 0; r < 8; r++)
                dma_copy_far(tb + (unsigned long)(cx * TROWS + cy) * 64 + r * 8,
                             lin + (unsigned long)(cy * 8 + r) * TPITCH + cx * 8, 8);
}

/* The letters' colours at frame t: letter i fades in from frame i * 10 over
 * 16 frames; from frame 90 a highlight sweeps across them, again and again */
static void title_colours(unsigned int t) {
    unsigned char i, b, l, fade, glow;
    for (i = 0; i < NLET; i++) {
        signed int s = (signed int)t - i * 10;
        fade = s <= 0 ? 0 : s >= 16 ? 16 : (unsigned char)s;
        glow = 0;
        if (t >= 90) {
            /* the highlight's centre: across the letters (32 a letter) and past them, every 200 frames */
            signed int p = (signed int)((t - 90) % 200) * 2 - 48, d = p - (i * 32 + 16);
            if (d < 0) d = -d;
            if (d < 32) glow = (unsigned char)(32 - d);
        }
        for (b = 0; b < BANDS; b++)
            for (l = 1; l <= 3; l++) {
                unsigned char rgb[3], k;
                for (k = 0; k < 3; k++) {
                    unsigned int base = band_rgb[b][k], c = base * l * fade / 48;
                    c += (255 - base) * l * glow / 96;    /* toward white, up to half */
                    rgb[k] = c > 255 ? 255 : (unsigned char)c;
                }
                palette_set_color(LETTERS + i * PER + b * 3 + l - 1, rgb[0], rgb[1], rgb[2]);
            }
    }
}

int main(void) {
    font_t ft, fb, fs;
    far_ptr_t mem, scratch;
    unsigned long at, screen, tb, bb, charset;
    unsigned int frames = 0;
    unsigned char line = 0, pos = 0;
    signed int tx = 2;
    char digits[8], one[2];

    palette_set_color(SEG, 60, 8, 8);         palette_set_color(SEG + 1, 150, 20, 20); palette_set_color(SEG + 2, 255, 70, 40);
    palette_set_color(BODY, 90, 90, 110);     palette_set_color(BODY + 1, 170, 170, 190); palette_set_color(BODY + 2, 255, 255, 255);
    palette_set_color(SHADOW, 10, 10, 30);    palette_set_color(SHADOW + 1, 10, 10, 30); palette_set_color(SHADOW + 2, 10, 10, 30);
    palette_set_color(OUTLINE, 20, 40, 110);  palette_set_color(OUTLINE + 1, 20, 40, 110); palette_set_color(OUTLINE + 2, 20, 40, 110);
    make_gradient();
    title_colours(0);

    bordercolor(0);
    bgcolor(0);
    clrscr();
    mem = far_malloc(46000u, BANK4);
    if (far_is_null(mem)) mem = far_malloc(46000u, BANK5);
    if (far_is_null(mem) || !fcm_enable()) {
        cputs("no room in banks 4 and 5\r\n");
        return 1;
    }
    /* conio's text in the 8x8 charset made from the same font */
    at = far_to_linear(mem);
    charset = (at + 2047) & ~2047UL;
    font_install(LIN(term), sizeof term, charset);
    font_select(charset);
    at = charset + sizeof term;

    if (!font_from(&ft, LIN(title)) || !font_from(&fs, place(score, sizeof score, &at)) || !font_from(&fb, LIN(body))) {
        fcm_disable();
        cputs("a font45 font was not read\r\n");
        return 1;
    }
    screen = (unsigned long)REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16);

    /* the title and the paragraph's blocks */
    tb = (at + 63) & ~63UL;
    bb = tb + (unsigned long)TCOLS * TROWS * 64;
    at = bb + (unsigned long)BCOLS * BROWS * 64;
    dma_fill_far(tb, (unsigned int)(at - tb), 0);
    /* (the title's linear buffer: only while it is drawn) */
    scratch = far_malloc(TPITCH * TROWS * 8, BANK5);
    if (far_is_null(scratch)) scratch = far_malloc(TPITCH * TROWS * 8, BANK4);
    if (far_is_null(scratch)) {
        fcm_disable();
        cputs("no room for the title's buffer\r\n");
        return 1;
    }
    dma_fill_far(far_to_linear(scratch), TPITCH * TROWS * 8, 0);
    draw_title(&ft, tb, far_to_linear(scratch));
    far_free(scratch);
    put_block(tb, TCOLS, TROWS, 2, 0);
    put_block(bb, BCOLS, BROWS, 3, 12);
    font_ramp(BODY, 3);

    cputsxy(31, 4, "font45: TrueType, BDF, PSF, images and charsets");
    cputsxy(31, 5, "for the MEGA65; <font.h> draws them");
    cputsxy(3, 18, "a 16-segment display's digits (font45 --segments 16 --fcm):");
    cputsxy(3, 23, "THIS TEXT IS CONIO'S, IN AN 8X8 CHARSET FONT45 MADE FROM THE SAME");
    cputsxy(3, 24, "TRUETYPE FONT AT 8 PIXELS (--CHARSET SCREENCODE --CENTER). ANY KEY QUITS.");

    one[1] = 0;
    for (;;) {
        title_colours(frames);
        /* the paragraph: a letter every other frame, from frame 50 */
        if (frames >= 50 && !(frames & 1) && line < 3) {
            one[0] = lines[line][pos];
            if (one[0]) {
                tx = font_draw_fcm(&fb, bb, BROWS, tx, 2 + line * 12, one);
                pos++;
            } else {
                line++;
                pos = 0;
                tx = 2;
            }
        }
        digits6(digits, frames);
        font_put_cells(&fs, screen, ROW, 3, 19, digits);
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
        /* the title's block at (2, 0); a letter's pixel in its gradient's bands
         * below the first; the counter's digit 9 has lit segments */
        unsigned int t = *(unsigned char __far *)(screen + 4) | (*(unsigned char __far *)(screen + 5) << 8);
        unsigned long nine = fs.glyphs + 9UL * fs.cols * fs.rows * 64, k;
        unsigned char lit = 0, banded = 0, outline = 0;
        for (k = 0; k < 576; k++) if (*(unsigned char __far *)(nine + k) == SEG + 2) lit = 1;
        for (k = 0; k < (unsigned long)TCOLS * TROWS * 64; k++) {
            unsigned char v = *(unsigned char __far *)(tb + k);
            if (v >= LETTERS + 3 && v < LETTERS + NLET * PER && (v - LETTERS) % PER >= 3) banded = 1;
            if (v == OUTLINE) outline = 1;
        }
        fcm_disable();
        clrscr();
        digits6(digits, frames);
        if (t == (unsigned int)(tb >> 6) && fb.bpp == 2 && ft.height == 80 && lit && banded && outline) {
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
