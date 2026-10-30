/* test_conio_cc65_xemu.c — the rest of cc65's conio (lib/stdlib_asm/conio.s45)
 *
 * Characters are read back as screen codes (getch_xy), colours from colour
 * RAM; keys are put into the KERNAL keyboard buffer ($02B0, count at $00D0).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] gotox / gotoy / screensize; cputc, cputcxy (moves the cursor);
 *       '\r' to column 0, "\r\n" one new row
 *   [1] textcolor / bgcolor / bordercolor return the old colour;
 *       textbackground is bgcolor
 *   [2] chline / chlinexy, cvline / cvlinexy (stopping at the bottom),
 *       cclear / cclearxy
 *   [3] cputhex8 / cputhex16
 *   [4] cpeekc / cpeekcolor / cpeekrevers / cpeeks (into the next row)
 *   [5] cprintf longer than 256 characters, vcprintf through a wrapper,
 *       %f (the float formatter linked by the float argument)
 *   [6] cursor; cgets with DEL, echoed, the cursor setting restored; cgetc
 *   [7] cscanf of a typed line
 *   [8] conio_set_width / conio_get_width
 *   [9] the earlier names: putch returns its character, getch, cursoron
 *  [10] $AA sentinel
 */
#pragma cc45 signed_default

#include <conio.h>
#include <string.h>
#include <stdarg.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define KEYBUF ((volatile unsigned char *)0x02B0)
#define KEYCOUNT (*(volatile unsigned char *)0x00D0)
#define REG(a) (*(volatile unsigned char *)(a))

static void keys(const char *k) {
    unsigned char n = 0;
    while (k[n]) { KEYBUF[n] = k[n]; n++; }
    KEYCOUNT = n;
}

static int t_cursor_out(void) {
    unsigned char w = 0, h = 0;
    int ok;
    clrscr();
    screensize(&w, &h);
    ok = w == 80 && h == 25;
    gotox(7);
    gotoy(3);
    ok = ok && wherex() == 7 && wherey() == 3;
    gotox(200);
    gotoy(200);
    ok = ok && wherex() == 79 && wherey() == 24;
    gotoxy(0, 0);
    cputc('A');
    cputcxy(5, 2, 'b');
    ok = ok && getch_xy(0, 0) == 0x41 && getch_xy(5, 2) == 0x02 && wherex() == 6 && wherey() == 2;
    cputs("xy\rz");
    ok = ok && getch_xy(0, 2) == 0x1A && getch_xy(6, 2) == 0x18 && wherex() == 1 && wherey() == 2;
    cputs("\r\n");
    return ok && wherex() == 0 && wherey() == 3;
}

static int t_colors(void) {
    unsigned char bg = REG(0xD021), border = REG(0xD020);
    int ok;
    textcolor(RED);
    ok = textcolor(GREEN) == RED;
    cputc('c');
    ok = ok && (getattr_xy(wherex() - 1, wherey()) & 0x0F) == GREEN;
    ok = ok && textcolor(WHITE) == GREEN;
    bgcolor(5);
    ok = ok && bgcolor(6) == 5 && REG(0xD021) == 6;
    ok = ok && textbackground(7) == 6 && REG(0xD021) == 7;
    bordercolor(2);
    ok = ok && bordercolor(3) == 2 && REG(0xD020) == 3;
    bgcolor(bg);
    bordercolor(border);
    return ok;
}

static int t_lines(void) {
    int ok;
    clrscr();
    gotoxy(2, 2);
    chline(3);
    ok = getch_xy(1, 2) == 0x20 && getch_xy(2, 2) == 0x40 && getch_xy(4, 2) == 0x40 && getch_xy(5, 2) == 0x20 &&
         wherex() == 5 && wherey() == 2;
    chlinexy(0, 3, 2);
    ok = ok && getch_xy(0, 3) == 0x40 && getch_xy(1, 3) == 0x40 && wherex() == 2;
    gotoxy(10, 10);
    cvline(3);
    ok = ok && getch_xy(10, 10) == 0x5D && getch_xy(10, 12) == 0x5D && getch_xy(10, 13) == 0x20 &&
         wherex() == 10 && wherey() == 13;
    cvlinexy(79, 22, 5);
    ok = ok && getch_xy(79, 22) == 0x5D && getch_xy(79, 24) == 0x5D && getch_xy(79, 21) == 0x20 && wherey() == 24;
    gotoxy(0, 5);
    cputs("abcd");
    cclearxy(1, 5, 2);
    ok = ok && getch_xy(0, 5) == 0x01 && getch_xy(1, 5) == 0x20 && getch_xy(2, 5) == 0x20 && getch_xy(3, 5) == 0x04 &&
         wherex() == 3;
    gotoxy(3, 5);
    cclear(1);
    return ok && getch_xy(3, 5) == 0x20 && wherex() == 4;
}

static int t_hex(void) {
    clrscr();
    cputhex8(0x3F);
    cputhex16(0xBE0A);
    /* '3' 'F' 'B' 'E' '0' 'A': F is screen code $46 (PETSCII $C6) */
    return getch_xy(0, 0) == 0x33 && getch_xy(1, 0) == 0x46 && getch_xy(2, 0) == 0x42 && getch_xy(3, 0) == 0x45 &&
           getch_xy(4, 0) == 0x30 && getch_xy(5, 0) == 0x41 && wherex() == 6;
}

static int t_peek(void) {
    char buf[8];
    int ok;
    clrscr();
    textcolor(CYAN);
    cputsxy(0, 4, "Hi!");
    revers(1);
    cputc('r');
    revers(0);
    textcolor(WHITE);
    gotoxy(0, 4);
    ok = cpeekc() == 'H' && cpeekcolor() == CYAN && cpeekrevers() == 0;
    gotoxy(3, 4);
    ok = ok && cpeekc() == 'r' && cpeekrevers() == 1;
    gotoxy(0, 4);
    cpeeks(buf, 4);
    ok = ok && strcmp(buf, "Hi!r") == 0 && wherex() == 0;
    cputsxy(78, 6, "xyz");
    gotoxy(78, 6);
    cpeeks(buf, 3);
    ok = ok && strcmp(buf, "xyz") == 0;
    /* none past the bottom */
    gotoxy(78, 24);
    cpeeks(buf, 5);
    return ok && strlen(buf) == 2;
}

static int myprintf(const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vcprintf(fmt, ap);
    va_end(ap);
    return n;
}

static char longs[301];

static int t_printf(void) {
    int ok, i;
    clrscr();
    for (i = 0; i < 300; i++) longs[i] = 'a';
    longs[300] = 0;
    ok = cprintf("%s", longs) == 300 && wherex() == 60 && wherey() == 3 && getch_xy(59, 3) == 0x01;
    gotoxy(0, 10);
    ok = ok && myprintf("v%d", 9) == 2 && getch_xy(0, 10) == 0x16 && getch_xy(1, 10) == 0x39;
    gotoxy(0, 11);
    ok = ok && cprintf("%.2f", 1.5) == 4 && getch_xy(0, 11) == 0x31 && getch_xy(1, 11) == 0x2E &&
         getch_xy(3, 11) == 0x30;
    return ok;
}

static int t_cgets(void) {
    char buf[10];
    char *r;
    int ok;
    clrscr();
    ok = cursor(1) == 0 && cursor(0) == 1;
    gotoxy(0, 2);
    keys("ab\x14" "c\r");
    r = cgets(buf, sizeof buf);
    ok = ok && r == buf && strcmp(buf, "ac") == 0 && getch_xy(0, 2) == 0x01 && getch_xy(1, 2) == 0x03 &&
         getch_xy(2, 2) == 0x20 && wherex() == 0 && wherey() == 3 && cursor(0) == 0;
    /* size - 1 characters at most */
    keys("wxyz");
    r = cgets(buf, 3);
    ok = ok && r == buf && strcmp(buf, "wx") == 0 && cgetc() == 'y' && cgetc() == 'z';
    /* nothing typed: NULL; bad arguments */
    keys("\r");
    ok = ok && cgets(buf, sizeof buf) == 0 && buf[0] == 0 && cgets(buf, 1) == 0 && cgets(0, 5) == 0;
    return ok;
}

static int t_cscanf(void) {
    int a = 0, b = 0, n;
    clrscr();
    keys("12 34\r");
    n = cscanf("%d %d", &a, &b);
    return n == 2 && a == 12 && b == 34 && getch_xy(0, 0) == 0x31 && wherey() == 1;
}

static int t_width(void) {
    int ok;
    ok = conio_get_width() == 80 && conio_set_width(40) == 1 && screenwidth() == 40 && conio_get_width() == 40;
    ok = ok && conio_set_width(60) == 0 && screenwidth() == 40;
    ok = ok && conio_set_width(80) == 1 && screenwidth() == 80;
    return ok;
}

static int t_old_names(void) {
    int ok;
    clrscr();
    ok = putch('m') == 'm' && getch_xy(0, 0) == 0x0D;
    keys("k");
    cursoron();
    ok = ok && getch() == 'k' && cursor(0) == 1;
    cursoroff();
    return ok;
}

int main(void) {
    OUT[0] = t_cursor_out();
    OUT[1] = t_colors();
    OUT[2] = t_lines();
    OUT[3] = t_hex();
    OUT[4] = t_peek();
    OUT[5] = t_printf();
    OUT[6] = t_cgets();
    OUT[7] = t_cscanf();
    OUT[8] = t_width();
    OUT[9] = t_old_names();
    OUT[10] = 0xAA;
    clrscr();
    return 0;
}
