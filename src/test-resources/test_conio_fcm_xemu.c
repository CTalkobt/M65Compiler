/* test_conio_fcm_xemu.c — conio.h's full-colour mode (a background colour
 * for each character) and conio_set_width
 *
 * Results at $C0C0 (01 = pass):
 *   [0] fcm_enable: CHR16 + FCLRHI, the stride twice the width, the text
 *       on the screen kept (characters and colours)
 *   [1] fcm_putch on the screen's background: the font's character; on
 *       another: a glyph >= 256 whose 64 bytes are the font's 'A' with $FF
 *       set and the background clear (black: $FE)
 *   [2] getch_xy / fcm_getattr_xy read a glyph cell back; the same
 *       character and background reuse the glyph
 *   [3] the other conio functions on the two-byte screen: cputsxy, clrscr,
 *       scrolling, getattr_xy
 *   [4] fcm_fill_rect, fcm_set_line_color, fcm_cputs, fcm_cprintf
 *   [5] a cache of 3: glyphs no cell shows are reused; full, a cell is
 *       drawn on the screen's background
 *   [6] fcm_disable: the one-byte screen back, glyph cells as their
 *       characters, the colours kept
 *   [7] conio_set_width 40 / 80, with and without fcm_enable (the stride
 *       doubled with it), the screen kept where it was; other widths refused
 *   [8] 01
 *   [9] $AA sentinel
 */
#include <conio.h>
#include <dma.h>
#include <stdio.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

__fastcall__ unsigned int __conio_get16(int x, int y);

static unsigned char pix[64];

static unsigned long scrptr(void) {
    return (unsigned long)REG(0xD060) | ((unsigned long)REG(0xD061) << 8) |
           ((unsigned long)REG(0xD062) << 16) | ((unsigned long)(REG(0xD063) & 0x0F) << 24);
}

/* glyph n's pixels are 'A' ($18 $3C $66 $7E $66 $66 $66 $00) in set / clear */
static int is_a(unsigned int n, unsigned char clear) {
    static const unsigned char a[8] = {0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00};
    unsigned char r, c, b;
    dma_copy_far((unsigned long)(unsigned int)pix, (unsigned long)n << 6, 64);
    for (r = 0; r < 8; r++) {
        b = a[r];
        for (c = 0; c < 8; c++) {
            if (pix[r * 8 + c] != ((b & 0x80) ? 0xFF : clear)) return 0;
            b <<= 1;
        }
    }
    return 1;
}

int main(void) {
    int ok;
    unsigned int n, n2, n3;
    unsigned long scr;

    REG(0xD021) = 6;    /* blue */
    textcolor(WHITE);
    clrscr();
    textcolor(YELLOW);
    cputsxy(0, 0, "hi");

    /* [0] */
    ok = fcm_enable() == 1 && fcm_is_enabled() && (REG(0xD054) & 5) == 5 &&
         REG(0xD058) == 160 && screenwidth() == 80;
    ok = ok && getch_xy(0, 0) == 8 && getch_xy(1, 0) == 9 && (getattr_xy(0, 0) & 15) == YELLOW &&
         __conio_get16(0, 0) == 8;
    OUT[0] = ok;

    /* [1] */
    fcm_putch(5, 1, 'a', RED, 6);           /* the screen's background */
    ok = __conio_get16(5, 1) == 1;
    fcm_putch(6, 1, 'a', RED, 2);           /* red background */
    n = __conio_get16(6, 1);
    ok = ok && n >= 256 && is_a(n, 2);
    fcm_putch(7, 1, 'a', WHITE, BLACK);
    n2 = __conio_get16(7, 1);
    ok = ok && n2 >= 256 && n2 != n && is_a(n2, 0xFE);
    OUT[1] = ok;

    /* [2] */
    ok = getch_xy(6, 1) == 1 && fcm_getattr_xy(6, 1) == 0x22 && fcm_getattr_xy(7, 1) == 0x01 &&
         fcm_getattr_xy(5, 1) == 0x62 && (getattr_xy(6, 1) & 15) == RED;
    fcm_putch(8, 1, 'a', GREEN, 2);
    ok = ok && __conio_get16(8, 1) == n && fcm_getattr_xy(8, 1) == 0x25;
    OUT[2] = ok;

    /* [3] */
    textcolor(CYAN);
    cputsxy(10, 2, "xy");
    ok = getch_xy(10, 2) == 24 && getch_xy(11, 2) == 25 && getattr_xy(10, 2) == CYAN &&
         __conio_get16(10, 2) == 24;
    gotoxy(0, 24);
    cputs("z\n");                           /* scrolls */
    ok = ok && getch_xy(0, 23) == 26 && getch_xy(10, 1) == 24 && __conio_get16(6, 0) == n &&
         getch_xy(0, 24) == 32 && __conio_get16(0, 24) == 32 && getattr_xy(0, 24) == CYAN;
    clrscr();
    ok = ok && __conio_get16(6, 0) == 32 && __conio_get16(79, 24) == 32 && getattr_xy(40, 12) == CYAN &&
         wherex() == 0 && wherey() == 0;
    OUT[3] = ok;

    /* [4] */
    fcm_fill_rect(2, 2, 4, 3, 'a', WHITE, 2);
    ok = __conio_get16(2, 2) == n && __conio_get16(4, 3) == n && __conio_get16(5, 3) == 32 &&
         __conio_get16(2, 4) == 32;
    fcm_cputs(78, 5, "ab", YELLOW, 6);      /* wraps */
    ok = ok && getch_xy(78, 5) == 1 && getch_xy(79, 5) == 2 && __conio_get16(78, 5) == 1;
    fcm_set_line_color(5, WHITE, 2);
    ok = ok && getch_xy(78, 5) == 1 && __conio_get16(78, 5) == n && fcm_getattr_xy(79, 5) == 0x21 &&
         fcm_getattr_xy(0, 5) == 0x21 && getch_xy(0, 5) == 32;
    ok = ok && fcm_cprintf(0, 6, GREEN, 7, "n%d", 42) == 3 && getch_xy(0, 6) == 14 &&
         getch_xy(1, 6) == '4' && getch_xy(2, 6) == '2' && fcm_getattr_xy(2, 6) == 0x75;
    OUT[4] = ok;

    /* [5] a cache of 3 */
    fcm_disable();
    clrscr();
    ok = fcm_set_cache_size(3) && fcm_enable();
    ok = ok && !fcm_set_cache_size(4);
    fcm_putch(0, 0, 'a', WHITE, 2);
    fcm_putch(1, 0, 'b', WHITE, 2);
    fcm_putch(2, 0, 'c', WHITE, 2);
    n = __conio_get16(0, 0);
    n2 = __conio_get16(1, 0);
    n3 = __conio_get16(2, 0);
    ok = ok && n >= 256 && n2 >= 256 && n3 >= 256 && n != n2 && n2 != n3;
    fcm_putch(3, 0, 'd', WHITE, 2);         /* full: on the screen's background */
    ok = ok && __conio_get16(3, 0) == 4 && fcm_getattr_xy(3, 0) == 0x61;
    fcm_putch(1, 0, 'b', WHITE, 6);         /* B's glyph no longer shown */
    fcm_putch(3, 0, 'd', WHITE, 2);
    ok = ok && __conio_get16(3, 0) == n2 && getch_xy(3, 0) == 4 && __conio_get16(0, 0) == n;
    OUT[5] = ok;

    /* [6] */
    scr = scrptr();
    fcm_disable();
    ok = !fcm_is_enabled() && (REG(0xD054) & 5) == 0 && REG(0xD058) == 80 && scrptr() == 0x800 &&
         scr != 0x800;
    ok = ok && getch_xy(0, 0) == 1 && getch_xy(3, 0) == 4 && getch_xy(1, 0) == 2 &&
         getattr_xy(0, 0) == WHITE;
    OUT[6] = ok;

    /* [7] */
    ok = !conio_set_width(64) && conio_set_width(40) && conio_get_width() == 40 && REG(0xD058) == 40 &&
         !(REG(0xD031) & 0x80) && scrptr() == 0x800;
    cputsxy(39, 0, "q");
    ok = ok && getch_xy(39, 0) == 17;
    ok = ok && fcm_enable() && REG(0xD058) == 80;
    scr = scrptr();
    ok = ok && conio_set_width(80) && REG(0xD058) == 160 && REG(0xD05E) == 80 && scrptr() == scr &&
         screenwidth() == 80;
    fcm_putch(79, 24, 'a', WHITE, 2);
    ok = ok && getch_xy(79, 24) == 1 && fcm_getattr_xy(79, 24) == 0x21;
    ok = ok && conio_set_width(40) && REG(0xD058) == 80 && screenwidth() == 40;
    fcm_putch(39, 0, 'a', WHITE, 2);
    ok = ok && getch_xy(39, 0) == 1;
    fcm_disable();                          /* enabled at 40 columns: that layout back */
    ok = ok && REG(0xD058) == 40 && getch_xy(39, 0) == 1 && scrptr() == 0x800;
    ok = ok && conio_set_width(80) && REG(0xD058) == 80 && screenwidth() == 80;
    OUT[7] = ok;

    OUT[8] = 1;
    OUT[9] = 0xAA;
    return 0;
}
