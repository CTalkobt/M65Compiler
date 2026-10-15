/* test_conio_xemu.c — conio.h on the MEGA65 text screen
 *
 * The screen as the VIC-IV shows it (80x25 in MEGA65 mode); characters are
 * read back as screen codes, colours from colour RAM.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] screenwidth / screenheight (80 x 25)
 *   [1] clrscr: spaces in the text colour, the cursor at (0, 0)
 *   [2] putch / cputs / cputch: screen codes ('A' $41, 'a' $01, '0' $30),
 *       the colour; getch_xy / getattr_xy read them back
 *   [3] '\n', wrapping at the end of a row, scrolling at the bottom
 *   [4] gotoxy (clamped) / wherex / wherey, cputsxy, cprintf, cprintfxy
 *   [5] textcolor, textattr, textbackground ($D021)
 *   [6] revers
 *   [7] kbhit / getch (keys put into the KERNAL keyboard buffer: $02B0,
 *       count at $00D0); cursoron/cursoroff: the cell under the cursor is
 *       restored after getch
 *   [8] other screens (the VIC-IV's registers set directly): 40 characters
 *       per row; 80 x 50 with screen RAM in bank 4 (above 64 KB) and colour
 *       RAM past its first 2 KB, scrolling there
 *   [9] $AA sentinel
 */
#pragma cc45 signed_default

#include <conio.h>
#include <string.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define KEYBUF ((volatile unsigned char *)0x02B0)
#define KEYCOUNT (*(volatile unsigned char *)0x00D0)

static int row_is(int y, const char *codes, int n) {
    int i;
    for (i = 0; i < n; i++)
        if (getch_xy(i, y) != (unsigned char)codes[i]) return 0;
    return 1;
}

static int t_size(void) {
    return screenwidth() == 80 && screenheight() == 25;
}

static int t_clrscr(void) {
    textcolor(LIGHT_GREEN);
    clrscr();
    return getch_xy(0, 0) == 0x20 && getch_xy(79, 24) == 0x20 && (getattr_xy(5, 5) & 0x0F) == LIGHT_GREEN &&
           wherex() == 0 && wherey() == 0;
}

static int t_put(void) {
    static const char want[] = { 0x41, 0x01, 0x30, 0x20, 0x21 };
    int ok;
    textcolor(YELLOW);
    clrscr();
    putch('A');
    cputs("a0 !");
    ok = row_is(0, want, 5) && wherex() == 5 && wherey() == 0 && (getattr_xy(1, 0) & 0x0F) == YELLOW;
    cputch(10, 3, 'Z');
    ok = ok && getch_xy(10, 3) == 0x5A && wherex() == 5;
    /* control characters print nothing */
    putch(0x05);
    return ok && wherex() == 5;
}

static int t_lines(void) {
    int ok, i;
    textcolor(WHITE);
    clrscr();
    cputs("ab\ncd");
    ok = getch_xy(0, 0) == 0x01 && getch_xy(0, 1) == 0x03 && wherex() == 2 && wherey() == 1;
    /* wrapping */
    gotoxy(78, 2);
    cputs("xyz");
    ok = ok && getch_xy(78, 2) == 0x18 && getch_xy(79, 2) == 0x19 && getch_xy(0, 3) == 0x1A && wherey() == 3;
    /* scrolling: the bottom row, then a newline */
    gotoxy(0, 24);
    cputs("last");
    putch('\n');
    ok = ok && wherey() == 24 && wherex() == 0 && getch_xy(0, 23) == 0x0C && getch_xy(0, 24) == 0x20 &&
         getch_xy(0, 0) == 0x03;   /* row 1 ("cd") moved up */
    for (i = 0; i < 30; i++) putch('\n');
    return ok && getch_xy(0, 0) == 0x20 && wherey() == 24;
}

static int t_pos(void) {
    int ok;
    clrscr();
    gotoxy(100, 40);
    ok = wherex() == 79 && wherey() == 24;
    gotoxy(-3, -1);
    ok = ok && wherex() == 0 && wherey() == 0;
    cputsxy(5, 6, "hi");
    ok = ok && getch_xy(5, 6) == 0x08 && getch_xy(6, 6) == 0x09 && wherex() == 7 && wherey() == 6;
    ok = ok && cprintf("%d", 42) == 2 && getch_xy(7, 6) == 0x34 && getch_xy(8, 6) == 0x32;
    ok = ok && cprintfxy(0, 10, "n=%d", -7) == 4 && getch_xy(0, 10) == 0x0E && getch_xy(3, 10) == 0x37;
    return ok;
}

static int t_colors(void) {
    unsigned char bg = *(volatile unsigned char *)0xD021;
    int ok;
    textcolor(RED);
    putch('q');
    ok = (getattr_xy(wherex() - 1, wherey()) & 0x0F) == RED;
    textattr((BLUE << 4) | ORANGE);
    putch('q');
    ok = ok && (getattr_xy(wherex() - 1, wherey()) & 0x0F) == ORANGE &&
         (*(volatile unsigned char *)0xD021 & 0x0F) == BLUE;
    textbackground(bg & 0x0F);
    return ok && (*(volatile unsigned char *)0xD021 & 0x0F) == (bg & 0x0F);
}

static int t_revers(void) {
    int ok;
    clrscr();
    ok = revers(1) == 0;
    putch('A');
    ok = ok && revers(0) == 1;
    putch('A');
    return ok && getch_xy(0, 0) == 0xC1 && getch_xy(1, 0) == 0x41;
}

static int t_keys(void) {
    int ok, a, b;
    clrscr();
    gotoxy(3, 3);
    putch('k');
    gotoxy(3, 3);
    KEYCOUNT = 0;
    ok = !kbhit();
    KEYBUF[0] = 0x41;    /* 'a' */
    KEYBUF[1] = 0x0D;
    KEYCOUNT = 2;
    ok = ok && kbhit();
    cursoron();
    a = getch();
    cursoroff();
    b = getch();
    return ok && a == 'a' && b == '\n' && !kbhit() && getch_xy(3, 3) == 0x0B;
}

static unsigned char peek_far(unsigned long a) {
    unsigned char c;
    dma_copy_far((unsigned long)(unsigned int)&c, a, 1);
    return c;
}

/* Other screens, set in the VIC-IV's registers (conio reads them on every
 * call): 40 characters per row; 80 x 50 with screen RAM in bank 4 (above
 * 64 KB; colour RAM past its first 2 KB), scrolling there */
static int t_geometry(void) {
    volatile unsigned char *v = (volatile unsigned char *)0xD000;
    static const unsigned char regs[] = { 0x58, 0x59, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x7B };
    unsigned char saved[8], i;
    unsigned long base, col;
    int ok;
    for (i = 0; i < 8; i++) saved[i] = v[regs[i]];
    base = v[0x60] | ((unsigned int)v[0x61] << 8) | ((unsigned long)v[0x62] << 16);
    col = 0xFF80000UL + (v[0x64] | ((unsigned int)v[0x65] << 8));
    /* 40 per row */
    v[0x58] = 40; v[0x59] = 0; v[0x5E] = 40;
    textcolor(WHITE);
    clrscr();
    gotoxy(38, 1);
    cputs("xyz");
    ok = screenwidth() == 40 && peek_far(base + 40 + 38) == 0x18 && peek_far(base + 40 + 39) == 0x19 &&
         peek_far(base + 80) == 0x1A && wherex() == 1 && wherey() == 2;
    /* 80 x 50 in bank 4 */
    v[0x58] = 80; v[0x5E] = 80; v[0x7B] = 49;
    v[0x60] = 0; v[0x61] = 0; v[0x62] = 4; v[0x63] = 0;
    textcolor(CYAN);
    clrscr();
    cputsxy(3, 49, "e");
    ok = ok && screenheight() == 50 && getch_xy(3, 49) == 0x05 && peek_far(0x40000UL + 49 * 80 + 3) == 0x05 &&
         (peek_far(col + 49 * 80 + 3) & 0x0F) == CYAN && (getattr_xy(3, 49) & 0x0F) == CYAN &&
         peek_far(0x40000UL + 4) == 0x20;
    gotoxy(0, 49);
    cputs("w\n");
    ok = ok && wherey() == 49 && peek_far(0x40000UL + 48 * 80) == 0x17 && peek_far(0x40000UL + 48 * 80 + 3) == 0x05 &&
         peek_far(0x40000UL + 49 * 80 + 3) == 0x20 && (peek_far(col + 48 * 80) & 0x0F) == CYAN;
    for (i = 0; i < 8; i++) v[regs[i]] = saved[i];
    textcolor(WHITE);
    clrscr();
    return ok;
}

int main(void) {
    OUT[0] = t_size();
    OUT[1] = t_clrscr();
    OUT[2] = t_put();
    OUT[3] = t_lines();
    OUT[4] = t_pos();
    OUT[5] = t_colors();
    OUT[6] = t_revers();
    OUT[7] = t_keys();
    textcolor(WHITE);
    clrscr();
    OUT[8] = t_geometry();
    OUT[9] = 0xAA;
    return 0;
}
