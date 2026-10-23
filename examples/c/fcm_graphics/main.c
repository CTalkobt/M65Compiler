/* fcm_graphics — full-colour characters on the MEGA65 (<conio.h>'s
 * full-colour mode)
 *
 * The VIC-IV's full-colour characters are 64 bytes, a byte a pixel: each
 * pixel a palette entry, not a bit. conio's full-colour mode (fcm_enable)
 * uses them to give every character cell its own background colour; a
 * program can show its own too (fcm_putglyph). This example shows both:
 *
 *   - a title and a table of the 16 colours, each character on its own
 *     background (fcm_putch / fcm_cputs / fcm_cprintf / fcm_fill_rect);
 *   - a plasma 32 x 12 characters (256 x 96 pixels) of 384 full-colour
 *     characters, each pixel one of 64 palette entries, and a bar of the 64
 *     entries; they move by palette cycling alone: each frame the 64 entries
 *     are rewritten one step further round, and no character changes.
 *
 * Keys: SPACE pauses, RUN/STOP quits. FCM_CHECK builds a test that runs 100
 * frames and reports on the screen (src/test/test_examples.sh).
 */
#include <conio.h>
#include <dma.h>
#include <far_ptr.h>
#include <input.h>

#define REG(a) (*(volatile unsigned char *)(a))

#define PX 1            /* the plasma's top left cell */
#define PY 2
#define PW 32           /* its size in cells */
#define PH 12
#define BARY 15         /* the palette bar's row */
#define RAMP 64         /* the palette entries it uses: RAMP .. RAMP + 63 */

/* sin, 0..63 over 64 steps */
static const unsigned char sine[64] = {
    32, 35, 38, 41, 44, 46, 49, 51, 54, 56, 58, 59, 61, 62, 62, 63,
    63, 63, 62, 62, 61, 59, 58, 56, 54, 51, 49, 46, 44, 41, 38, 35,
    32, 28, 25, 22, 19, 17, 14, 12,  9,  7,  5,  4,  2,  1,  1,  0,
     0,  0,  1,  1,  2,  4,  5,  7,  9, 12, 14, 17, 19, 22, 25, 28,
};

/* A colour wheel: red, green and blue are this wave a third apart */
static const unsigned char wave[64] = {
    128, 140, 152, 165, 176, 188, 198, 208, 218, 226, 234, 240, 245, 250, 253, 254,
    255, 254, 253, 250, 245, 240, 234, 226, 218, 208, 198, 188, 176, 165, 152, 140,
    128, 115, 103,  90,  79,  67,  57,  47,  37,  29,  21,  15,  10,   5,   2,   1,
      0,   1,   2,   5,  10,  15,  21,  29,  37,  47,  57,  67,  79,  90, 103, 115,
};

static const char *const names[16] = {
    "black", "white", "red", "cyan", "purple", "green", "blue", "yellow",
    "orange", "brown", "light red", "dark grey", "grey", "light green", "light blue", "light grey",
};

static unsigned char pix[64];

/* A palette register's value: the VIC-IV's have their nibbles swapped */
#define NYB(v) ((unsigned char)(((v) << 4) | ((v) >> 4)))

/* Palette entries RAMP .. RAMP + 63: the colour wheel from step at */
static void cycle(unsigned char at) {
    unsigned char i, k;
    for (i = 0; i < 64; i++) {
        k = (unsigned char)(i + at);
        REG(0xD100 + RAMP + i) = NYB(wave[k & 63]);
        REG(0xD200 + RAMP + i) = NYB(wave[(k + 21) & 63]);
        REG(0xD300 + RAMP + i) = NYB(wave[(k + 43) & 63]);
    }
}

/* One full-colour character's 64 pixels into memory at a (DMA) */
static void store(unsigned long a) {
    dma_copy_far(a, (unsigned long)(unsigned int)pix, 64);
}

/* The plasma's characters and the bar's, from a (64-byte aligned): the
 * character at a + 64 * k is number a / 64 + k */
static void make_glyphs(unsigned long a) {
    unsigned char cx, cy, r, c, px, py, i;
    for (cy = 0; cy < PH; cy++)
        for (cx = 0; cx < PW; cx++) {
            i = 0;
            for (r = 0; r < 8; r++) {
                py = (unsigned char)(cy * 8 + r);
                for (c = 0; c < 8; c++) {
                    px = (unsigned char)(cx * 8 + c);
                    pix[i++] = (unsigned char)(RAMP +
                        ((sine[(px >> 1) & 63] + sine[(py + (px >> 2)) & 63] +
                          sine[((px + py) >> 1) & 63] + sine[(sine[py & 63] + (px >> 1)) & 63]) >> 2));
                }
            }
            store(a);
            a += 64;
        }
    /* the bar: 32 characters, 2 entries each (4 pixels wide) */
    for (cx = 0; cx < 32; cx++) {
        for (i = 0; i < 64; i++) pix[i] = (unsigned char)(RAMP + cx * 2 + ((i & 7) >> 2));
        store(a);
        a += 64;
    }
}

/* Text in a contrasting colour on bg */
static int ink(int bg) {
    return (bg == 0 || bg == 2 || bg == 4 || bg == 6 || bg == 9 || bg == 11) ? 1 : 0;
}

static void draw_text(void) {
    static const char title[] = "full-colour characters on the mega65";
    int i, x, bg = 2;
    for (i = 0; title[i]; i++) {
        if (title[i] == ' ') {
            fcm_putch(i + 1, 0, ' ', 1, 0);
            continue;
        }
        fcm_putch(i + 1, 0, title[i], ink(bg), bg);
        bg = bg == 15 ? 2 : bg + 1;     /* (not 0, the screen's) */
    }
    fcm_cputs(38, 2, "every character its own background:", 15, 0);
    for (i = 0; i < 16; i++) {
        x = 38 + (i / 8) * 21;
        fcm_fill_rect(x, 4 + i % 8, x + 19, 4 + i % 8, ' ', 1, i);
        fcm_cprintf(x + 1, 4 + i % 8, ink(i), i, "%2d %s", i, names[i]);
    }
    fcm_cputs(38, 13, "the plasma: 384 full-colour characters,", 15, 0);
    fcm_cputs(38, 14, "a byte a pixel, 64 palette entries", 15, 0);
    fcm_cputs(38, 15, "rewritten each frame: nothing else", 15, 0);
    fcm_cputs(38, 16, "changes on the screen.", 15, 0);
    fcm_cputs(PX, BARY + 2, "palette entries 64-127", 12, 0);
    fcm_cputs(1, 24, " space ", 0, 7);
    fcm_cputs(8, 24, " pause   ", 1, 0);
    fcm_cputs(17, 24, " run/stop ", 0, 7);
    fcm_cputs(27, 24, " quit", 1, 0);
}

static void wait_frame(void) {
    while (REG(0xD012) != 0xF8) ;
    while (REG(0xD012) == 0xF8) ;
}

int main(void) {
    unsigned char border = REG(0xD020), screen = REG(0xD021), at = 0, paused = 0;
    unsigned int frames = 0, first, k;
    unsigned char cx, cy;
    far_ptr_t mem;
    unsigned long a;
    int key;

    REG(0xD020) = 0;
    REG(0xD021) = 0;
    textcolor(1);
    clrscr();
    /* 384 + 32 characters, 64-byte aligned */
    mem = far_malloc((PW * PH + 32) * 64u + 63u, BANK4);
    if (far_is_null(mem)) mem = far_malloc((PW * PH + 32) * 64u + 63u, BANK5);
    if (far_is_null(mem) || !fcm_enable()) {
        cputs("not enough far memory\n");
        return 1;
    }
    a = (far_to_linear(mem) + 63) & ~63UL;
    first = (unsigned int)(a >> 6);
    cycle(0);
    make_glyphs(a);
    k = first;
    for (cy = 0; cy < PH; cy++)
        for (cx = 0; cx < PW; cx++)
            fcm_putglyph(PX + cx, PY + cy, k++, 1);
    for (cx = 0; cx < 32; cx++) fcm_putglyph(PX + cx, BARY, k++, 1);
    draw_text();

    for (;;) {
        wait_frame();
        if (!paused) {
            cycle(++at);
            frames++;
        }
        fcm_cprintf(PX + 24, BARY + 2, 11, 0, "%5u", frames);
#ifdef FCM_CHECK
        if (frames == 100) break;
#endif
        if (kbhit()) {
            key = getch();
            if (key == ' ') paused = !paused;
            if (key == CH_STOP) break;
        }
    }

#ifdef FCM_CHECK
    {
        /* the title's cells: their characters and colours; the plasma's
         * own characters are not in the cache */
        int ok = getch_xy(1, 0) == 6 && fcm_getattr_xy(1, 0) == 0x21 && getch_xy(12, 0) == ' ' &&
                 fcm_getattr_xy(39, 4) == 0x01 && fcm_getattr_xy(59, 11) == 0xF1 && fcm_getattr_xy(60, 11) == 0xF0;
        fcm_disable();
        ok = ok && getch_xy(1, 0) == 6 && getch_xy(PX, PY) == ' ';
        gotoxy(0, 22);
        cprintf(ok ? "fcm check ok, %u frames\n" : "fcm check failed\n", frames);
    }
#else
    fcm_disable();
    clrscr();
#endif
    far_free(mem);
    REG(0xD020) = border;
    REG(0xD021) = screen;
    return 0;
}
