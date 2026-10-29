/* test_tgi_ncm_xemu.c — <tgi.h>'s 640x200, 16-colour drivers
 * (tgi_static_ncm640, tgi_static_ncm2: nibble-colour characters at H640)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] tgi_install: its numbers (640x200, 16 colours, 1 page)
 *   [1] tgi_init: H640, every character's colour RAM $08 (nibble colour) $0F
 *   [2] pixels: set and read back, the left one the low nibble of its byte
 *       (at pixel / 16 * 1600 + y * 8 + pixel / 2 % 8), the neighbours kept
 *   [3] off the screen: nothing drawn, 0 read
 *   [4] lines: across the screen, down it, a diagonal, clipped
 *   [5] bars: odd and even edges, whole character columns, clipped
 *   [6] text: a glyph at 1x (at an odd x) and at 2x, the same pixels
 *   [7] a circle's points
 *   [8] tgi_setpalette: 16 entries (the others the default)
 *   [9] tgi_uninstall: H640 as it was; the 320x200 driver after it
 *   [10] the two-page driver: two pages in banks 4 and 5, drawn and shown
 *        apart (the screen pointer, the pixels of each), by name too
 *   [11] $AA sentinel
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define R(a) (*(volatile unsigned char *)(a))

static unsigned long page;

static unsigned char nib(unsigned int x, unsigned char y) {
    const unsigned char __far *p = (const unsigned char __far *)
        (page + (unsigned long)(x >> 4) * 1600u + (unsigned int)y * 8u + ((x >> 1) & 7u));
    return (unsigned char)((x & 1) ? *p >> 4 : *p & 15);
}

/* pixels of colour c in the box */
static unsigned int count(int x1, int y1, int x2, int y2, unsigned char c) {
    int x, y;
    unsigned int n = 0;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            if (tgi_getpixel(x, y) == c) n++;
    return n;
}

int main(void) {
    unsigned char ok, d031 = R(0xD031), i, a, b;
    unsigned int k;
    unsigned char __far *cr = (unsigned char __far *)0xFF83000UL;
    static unsigned char pal[16];
    int x;

    ok = tgi_install(tgi_static_ncm640) == TGI_ERR_OK && tgi_getmaxx() == 639 && tgi_getmaxy() == 199 &&
         tgi_getxres() == 640 && tgi_getcolorcount() == 16 && tgi_getmaxcolor() == 15 &&
         tgi_getpagecount() == 1;
    OUT[0] = ok;

    tgi_init();
    tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &page);
    ok = (R(0xD031) & 0x80) && tgi_geterror() == TGI_ERR_OK;
    for (k = 0; k < 2000; k += 2)
        if (cr[k] != 0x08 || cr[k + 1] != 0x0F) ok = 0;
    OUT[1] = ok;

    /* pixels */
    tgi_setcolor(3); tgi_setpixel(0, 0);
    tgi_setcolor(5); tgi_setpixel(1, 0);
    tgi_setcolor(15); tgi_setpixel(639, 199);
    tgi_setcolor(9); tgi_setpixel(638, 199);
    tgi_setcolor(12); tgi_setpixel(17, 5);
    ok = tgi_getpixel(0, 0) == 3 && tgi_getpixel(1, 0) == 5 && tgi_getpixel(639, 199) == 15 &&
         tgi_getpixel(638, 199) == 9 && tgi_getpixel(17, 5) == 12 && tgi_getpixel(16, 5) == 0 &&
         tgi_getpixel(18, 5) == 0 && *(unsigned char __far *)page == 0x53 &&
         nib(17, 5) == 12 && nib(639, 199) == 15 &&
         *(unsigned char __far *)(page + 1600u + 5u * 8u) == 0xC0;
    tgi_setcolor(16);                               /* not a colour: kept */
    ok = ok && tgi_geterror() == TGI_ERR_INV_ARG && tgi_getcolor() == 12;
    OUT[2] = ok;

    /* off the screen */
    tgi_setcolor(7);
    tgi_setpixel(640, 0); tgi_setpixel(-1, 0); tgi_setpixel(0, 200); tgi_setpixel(0, -1);
    ok = tgi_getpixel(640, 0) == 0 && tgi_getpixel(-1, 5) == 0 && tgi_getpixel(5, 200) == 0 &&
         count(0, 0, 3, 3, 7) == 0;
    OUT[3] = ok;

    /* lines */
    tgi_clear();
    tgi_setcolor(4); tgi_line(0, 10, 639, 10);
    tgi_setcolor(6); tgi_line(321, 0, 321, 199);
    tgi_setcolor(8); tgi_line(100, 100, 150, 150);
    tgi_setcolor(2); tgi_line(-50, 60, 700, 60);    /* clipped */
    ok = count(0, 10, 639, 10, 4) == 639 && tgi_getpixel(321, 10) == 6 &&
         count(321, 0, 321, 199, 6) == 199 && count(100, 100, 150, 150, 8) == 51 &&
         tgi_getpixel(125, 125) == 8 && tgi_getpixel(126, 125) == 0 &&
         count(0, 60, 639, 60, 2) == 640 && tgi_getpixel(321, 60) == 2;
    OUT[4] = ok;

    /* bars */
    tgi_clear();
    tgi_setcolor(7); tgi_bar(3, 20, 40, 30);        /* odd left, even right */
    tgi_setcolor(11); tgi_bar(64, 40, 95, 41);      /* whole character columns */
    tgi_setcolor(13); tgi_bar(630, 190, 700, 250);  /* clipped */
    ok = count(0, 18, 45, 32, 7) == 38 * 11 && tgi_getpixel(3, 20) == 7 && tgi_getpixel(2, 20) == 0 &&
         tgi_getpixel(40, 30) == 7 && tgi_getpixel(41, 30) == 0 && tgi_getpixel(40, 31) == 0 &&
         count(60, 38, 100, 43, 11) == 64 && tgi_getpixel(63, 40) == 0 && tgi_getpixel(96, 40) == 0 &&
         count(620, 185, 639, 199, 13) == 10 * 10;
    OUT[5] = ok;

    /* text: 'A' at 1x at an odd x and at 2x: 2x has 4 pixels for each */
    tgi_clear();
    tgi_setcolor(10);
    tgi_outtextxy(101, 50, "A");
    tgi_settextstyle(0x200, 0x200, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
    tgi_outtextxy(301, 50, "A");
    {
        unsigned int n1 = count(101, 50, 108, 57, 10), n2 = count(301, 50, 316, 65, 10);
        ok = n1 > 8 && n2 == 4 * n1 && count(90, 40, 120, 70, 10) == n1;
        for (x = 0; x < 8; x++)                     /* the same shape */
            for (i = 0; i < 8; i++)
                if ((tgi_getpixel(101 + x, 50 + i) == 10) != (tgi_getpixel(301 + 2 * x, 50 + 2 * i) == 10)) ok = 0;
    }
    OUT[6] = ok;
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);

    /* a circle */
    tgi_clear();
    tgi_setcolor(14); tgi_circle(320, 100, 50);
    ok = tgi_getpixel(370, 100) == 14 && tgi_getpixel(270, 100) == 14 && tgi_getpixel(320, 50) == 14 &&
         tgi_getpixel(320, 150) == 14 && tgi_getpixel(320, 100) == 0 && count(260, 40, 380, 160, 14) > 200;
    OUT[7] = ok;

    /* the palette: 16 entries */
    for (i = 0; i < 16; i++) pal[i] = (unsigned char)(15 - i);
    a = R(0xD10F); b = R(0xD110);
    tgi_setpalette(pal);
    ok = R(0xD100) == a && R(0xD110) == b && tgi_getpalette()[0] == 15 && tgi_getpalette()[16] == 16;
    OUT[8] = ok;

    tgi_uninstall();
    ok = R(0xD031) == d031 && tgi_install(tgi_static_stddrv) == TGI_ERR_OK && tgi_getmaxx() == 319 &&
         tgi_getcolorcount() == 0;
    tgi_init();
    tgi_setcolor(200); tgi_setpixel(5, 5); tgi_setcolor(201); tgi_setpixel(6, 5);
    ok = ok && tgi_getpixel(5, 5) == 200 && tgi_getpixel(6, 5) == 201;
    tgi_uninstall();
    OUT[9] = ok;

    /* two pages */
    {
        unsigned long p0 = 0, p1 = 0, v = 0;
        unsigned char s0[3];
        ok = tgi_install(tgi_static_ncm2) == TGI_ERR_OK && tgi_getpagecount() == 2 && tgi_getmaxx() == 639;
        tgi_init();
        tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &p0);
        tgi_setcolor(9); tgi_setpixel(101, 7); tgi_line(0, 199, 639, 199);
        s0[0] = R(0xD060); s0[1] = R(0xD061); s0[2] = R(0xD062);
        tgi_setdrawpage(1);
        tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &p1);
        tgi_setcolor(4); tgi_setpixel(102, 7);
        ok = ok && p1 != p0 && (unsigned char)(p0 >> 16) >= 4 && (unsigned char)(p1 >> 16) >= 4 &&
             tgi_getpixel(102, 7) == 4 && tgi_getpixel(101, 7) == 0 && tgi_getpixel(5, 199) == 0;
        tgi_setdrawpage(0);
        ok = ok && tgi_getpixel(101, 7) == 9 && tgi_getpixel(102, 7) == 0 && count(0, 199, 639, 199, 9) == 640;
        tgi_setviewpage(1);
        tgi_ioctl(TGI_IOCTL_VIEWPAGE_ADDR, &v);
        ok = ok && v == p1 && (R(0xD060) != s0[0] || R(0xD061) != s0[1] || R(0xD062) != s0[2]) &&
             (R(0xD031) & 0x80);
        tgi_setviewpage(0);
        ok = ok && R(0xD060) == s0[0] && R(0xD061) == s0[1] && R(0xD062) == s0[2];
        tgi_uninstall();
        tgi_load_driver("mega65-ncm2.tgi");
        ok = ok && tgi_geterror() == TGI_ERR_OK && tgi_getpagecount() == 2 && tgi_getmaxx() == 639;
        tgi_uninstall();
        ok = ok && R(0xD031) == d031;
    }
    OUT[10] = ok;

    OUT[11] = 0xAA;
    return 0;
}
