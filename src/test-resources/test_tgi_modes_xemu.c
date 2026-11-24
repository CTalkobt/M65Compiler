/* test_tgi_modes_xemu.c — <tgi.h>'s generic drivers (#645): the text
 * screens (block characters, text, tgi_toggle_case) and the VIC-II bitmap
 * modes, through the shared drawing (lines, bars, circles, patterns, bitmap
 * text, tgi_copyrect)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] mega65-text-40x25: install, 80x50, a quarter's block character
 *       (upper left $7E, lower right $6C), getpixel (the cell's colour, a
 *       quarter off: 0), tgi_outtextxy (screen codes in the cells), a line
 *       (top halves $E2), a bar (full $A0)
 *   [1] mega65-text-80x50: 160x100, H640 and V400, LINESTEP 80, 50 rows;
 *       the last cell; text; tgi_toggle_case (lower case: $D069 bit 3);
 *       tgi_done: the screen's size and case back
 *   [2] mega65-hires: the bitmap mode ($D011 bit 5), 8 KB aligned; a pixel
 *       (its bit, the cell's foreground), getpixel; tgi_done: $D011 back
 *   [3] mega65-mcm: multicolour ($D016 bit 4); four colours in a cell (the
 *       pairs %01 %10 %11 %01, the colour byte's nibbles, colour RAM),
 *       getpixel, a pixel in 0 (%00)
 *   [4] the shared drawing on hires (by name, tgi_load_driver): a circle,
 *       a patterned bar, bitmap text (the font saved before the bitmap
 *       took the character pointer), tgi_copyrect; text-80x50 by name
 *   [5] $AA sentinel
 */
#include <tgi.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

extern unsigned long __tgi_b_bmp, __tgi_b_scr;

static unsigned char __far *screen(void) {
    return (unsigned char __far *)(REG(0xD060) | ((unsigned long)REG(0xD061) << 8) |
                                   ((unsigned long)REG(0xD062) << 16));
}

static unsigned int count(int x1, int y1, int x2, int y2) {
    unsigned int n = 0;
    int x, y;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            if (tgi_getpixel(x, y)) n++;
    return n;
}

static const unsigned char pat[8] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };

static int t_text40(void) {
    unsigned char __far *scr;
    int ok;
    tgi_install(tgi_static_text40);
    tgi_init();
    ok = tgi_geterror() == 0 && tgi_getxres() == 80 && tgi_getyres() == 50;
    scr = screen();
    tgi_setcolor(5);
    tgi_setpixel(0, 0);
    tgi_setpixel(3, 1);
    ok = ok && scr[0] == 0x7E && scr[1] == 0x6C;
    ok = ok && tgi_getpixel(0, 0) == 5 && tgi_getpixel(1, 0) == 0 && tgi_getpixel(3, 1) == 5;
    tgi_outtextxy(0, 4, "hi");
    ok = ok && scr[80] == 0x08 && scr[81] == 0x09;
    tgi_line(0, 10, 9, 10);
    ok = ok && scr[200] == 0xE2 && scr[204] == 0xE2;
    tgi_bar(20, 20, 23, 23);
    ok = ok && scr[410] == 0xA0 && scr[451] == 0xA0;
    tgi_done();
    tgi_uninstall();
    return ok;
}

static int t_text80(void) {
    unsigned char __far *scr;
    unsigned char d031 = REG(0xD031) & 0x88;
    int ok;
    tgi_install(tgi_static_text80);
    tgi_init();
    ok = tgi_geterror() == 0 && tgi_getxres() == 160 && tgi_getyres() == 100;
    ok = ok && (REG(0xD031) & 0x88) == 0x88 && REG(0xD058) == 80 && REG(0xD07B) == 49;
    scr = screen();
    tgi_setcolor(2);
    tgi_setpixel(159, 99);
    ok = ok && scr[49 * 80 + 79] == 0x6C && tgi_getpixel(159, 99) == 2;
    tgi_outtextxy(2, 2, "a");
    ok = ok && scr[81] == 0x01;
    ok = ok && tgi_toggle_case() == 1 && (REG(0xD069) & 0x08) && tgi_getpixel(159, 99) == 2;
    tgi_done();
    ok = ok && (REG(0xD031) & 0x88) == d031 && !(REG(0xD069) & 0x08);
    tgi_uninstall();
    return ok;
}

static int t_hires(void) {
    unsigned char __far *b;
    unsigned char __far *c;
    unsigned char d011 = REG(0xD011);
    int ok;
    tgi_install(tgi_static_hires);
    tgi_init();
    ok = tgi_geterror() == 0 && (REG(0xD011) & 0x20) && (__tgi_b_bmp & 0x1FFF) == 0;
    b = (unsigned char __far *)__tgi_b_bmp;
    c = (unsigned char __far *)__tgi_b_scr;
    tgi_setcolor(7);
    tgi_setpixel(9, 10);
    ok = ok && b[330] == 0x40 && c[41] == 0x70 && tgi_getpixel(9, 10) == 7 && tgi_getpixel(10, 10) == 0;
    tgi_done();
    ok = ok && REG(0xD011) == d011;
    tgi_uninstall();
    return ok;
}

static int t_mcm(void) {
    unsigned char __far *b;
    unsigned char __far *c;
    int ok;
    tgi_install(tgi_static_mcm);
    tgi_init();
    ok = tgi_geterror() == 0 && (REG(0xD016) & 0x10) && tgi_getxres() == 160;
    b = (unsigned char __far *)__tgi_b_bmp;
    c = (unsigned char __far *)__tgi_b_scr;
    tgi_setcolor(2); tgi_setpixel(0, 0);
    tgi_setcolor(5); tgi_setpixel(1, 0);
    tgi_setcolor(6); tgi_setpixel(2, 0);
    tgi_setcolor(2); tgi_setpixel(3, 0);
    ok = ok && b[0] == 0x6D && c[0] == 0x25;
    ok = ok && tgi_getpixel(0, 0) == 2 && tgi_getpixel(1, 0) == 5 && tgi_getpixel(2, 0) == 6 && tgi_getpixel(3, 0) == 2;
    tgi_setcolor(0); tgi_setpixel(1, 0);
    ok = ok && b[0] == 0x4D && tgi_getpixel(1, 0) == 0;
    tgi_done();
    ok = ok && !(REG(0xD016) & 0x10);
    tgi_uninstall();
    return ok;
}

static int t_shared(void) {
    int ok;
    tgi_load_driver("mega65-hires.tgi");
    ok = tgi_geterror() == 0;
    tgi_init();
    tgi_setcolor(1);
    tgi_circle(100, 100, 20);
    ok = ok && tgi_getpixel(120, 100) && tgi_getpixel(80, 100) && tgi_getpixel(100, 80) && tgi_getpixel(100, 120) &&
         !tgi_getpixel(100, 100) && count(78, 78, 122, 122) > 100;
    tgi_setpattern_mono(pat, 1, 0);
    tgi_bar(200, 10, 215, 25);
    ok = ok && count(200, 10, 215, 25) == 128;
    tgi_setsolid();
    tgi_outtextxy(10, 150, "H");
    ok = ok && count(10, 150, 17, 157) == 30;
    tgi_copyrect(10, 150, 17, 157, 30, 150);
    ok = ok && count(30, 150, 37, 157) == 30 && tgi_getpixel(11, 150) == tgi_getpixel(31, 150);
    tgi_done();
    tgi_uninstall();
    tgi_load_driver("MEGA65-TEXT-80X50.TGI");
    ok = ok && tgi_geterror() == 0;
    tgi_uninstall();
    return ok;
}

int main(void) {
    OUT[5] = 0;
    OUT[0] = t_text40();
    OUT[1] = t_text80();
    OUT[2] = t_hires();
    OUT[3] = t_mcm();
    OUT[4] = t_shared();
    OUT[5] = 0xAA;
    return 0;
}
