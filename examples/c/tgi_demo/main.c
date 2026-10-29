/* tgi_demo — cc65's TGI (<tgi.h>) on the MEGA65
 *
 * Written as a cc65 TGI program is: the driver installed, tgi_init, and
 * only tgi_* calls after that, so it ports to and from the other TGI
 * platforms with its calls unchanged. The drawing follows the driver's
 * resolution and colours (tgi_getmaxx, tgi_getcolorcount).
 *
 * Twice, at 320x200 in 256 colours ("mega65-fcm.tgi", "mega65-fcm2.tgi")
 * and at 640x200 in 16 ("mega65-ncm640.tgi", "mega65-ncm2.tgi"): first the
 * drawing functions on the one-page driver (the palette, lines clipped at
 * the screen's edge, circle, ellipse, arc and pie slice, bitmap text
 * magnified and turned); a key goes on to a star turning on the two-page
 * driver: each frame is drawn on the page not shown, then shown
 * (tgi_setdrawpage / tgi_setviewpage), so no frame is seen half drawn. A
 * key goes on; tgi_unload puts the text screen back at the end.
 *
 * TGI_CHECK (make check) runs without keys: both modes, 50 frames each,
 * then reports on the text screen. TGI_640_ONLY (make demo640): the
 * 640x200 part only.
 */
#include <tgi.h>
#include <conio.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))

/* cos(i * 360 / 64) in 8.8 */
static const signed int cos64[64] = {
    256, 255, 251, 245, 237, 226, 213, 198, 181, 162, 142, 121, 98, 74, 50, 25,
    0, -25, -50, -74, -98, -121, -142, -162, -181, -198, -213, -226, -237, -245, -251, -255,
    -256, -255, -251, -245, -237, -226, -213, -198, -181, -162, -142, -121, -98, -74, -50, -25,
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255
};

/* x at 320 pixels across, at the driver's width (640: doubled; the 640x200
 * pixel is half as wide, so a circle's x radius is doubled too) */
static unsigned char wide;
static int X(int x) {
    return wide ? 2 * x : x;
}

static void wait_frame(void) {
    while (REG(0xD012) != 0xF8) ;
    while (REG(0xD012) == 0xF8) ;
}

static void showcase(void) {
    unsigned int i, n = tgi_getcolorcount();
    unsigned int maxx = tgi_getmaxx(), maxy = tgi_getmaxy();
    tgi_clear();
    /* the first 32 palette entries (or all 16) */
    if (n == 0 || n > 32) n = 32;
    for (i = 0; i < n; i++) {
        tgi_setcolor((unsigned char)i);
        tgi_bar(X(192 + (i & 15) * 8), 2 + (i >> 4) * 8, X(199 + (i & 15) * 8) + (wide ? 1 : 0), 9 + (i >> 4) * 8);
    }
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_line(0, 0, maxx, 0);
    tgi_lineto(maxx, maxy);
    tgi_lineto(0, maxy);
    tgi_lineto(0, 0);
    /* a fan of lines, clipped at the top */
    for (i = 0; i <= 15; i++) {
        tgi_setcolor((unsigned char)(2 + i % 14));
        tgi_line(X(60), 60, X(60 + tgi_imulround(90, (signed int)(i * 24) - 180)), -40);
    }
    tgi_setcolor(TGI_COLOR_YELLOW);
    if (wide) tgi_ellipse(X(60), 150, 80, 40);
    else tgi_circle(60, 150, 40);
    tgi_setcolor(TGI_COLOR_LIGHTGREEN);
    tgi_ellipse(X(140), 150, (unsigned char)X(40), 20);
    tgi_setcolor(TGI_COLOR_CYAN);
    tgi_pieslice(X(250), 155, (unsigned char)X(40), 30, 30, 120);
    tgi_setcolor(TGI_COLOR_LIGHTRED);
    tgi_arc(X(250), 155, (unsigned char)X(40), 30, 180, 330);
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_outtextxy(8, 78, "TGI on the MEGA65");
    tgi_settextscale(0x200, 0x200);
    tgi_setcolor(TGI_COLOR_ORANGE);
    tgi_outtextxy(8, 90, wide ? "640x200" : "320x200");
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_VERTICAL, TGI_FONT_BITMAP);
    tgi_setcolor(TGI_COLOR_LIGHTBLUE);
    tgi_outtextxy(X(300), 190, wide ? "16 colours" : "256 colours");
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
    tgi_setcolor(TGI_COLOR_GRAY3);
    tgi_outtextxy(X(110), 188, "Press a key");
}

/* A five-pointed star turned by a (of 64): 10 corners, points and
 * notches */
static void star(unsigned char a, unsigned char colour) {
    unsigned char k, p;
    int x, y;
    tgi_setcolor(colour);
    for (k = 0; k <= 10; k++) {
        p = (unsigned char)((a + k * 32u / 5u) & 63);
        x = X(160 + tgi_imulround((k & 1) ? 40 : 90, cos64[p]));
        y = 100 - tgi_imulround((k & 1) ? 40 : 90, cos64[(p + 48) & 63]);
        if (k == 0) tgi_gotoxy(x, y);
        else tgi_lineto(x, y);
    }
}

/* Frames of the turning star until a key (or n frames: TGI_CHECK) */
static unsigned int animate(unsigned int n) {
    unsigned char page = 1, a = 0;
    unsigned int frames = 0;
    while (1) {
        tgi_setdrawpage(page);
        tgi_clear();
        star(a, (unsigned char)(2 + (a >> 2) % 14));
        star((unsigned char)(64 - a), TGI_COLOR_WHITE);
        tgi_outtextxy(4, 4, "Double buffered: two pages");
        tgi_outtextxy(4, 190, "Press a key to end");
        wait_frame();
        tgi_setviewpage(page);
        page ^= 1;
        a = (unsigned char)((a + 1) & 63);
        frames++;
        if (n ? frames >= n : kbhit()) break;
    }
    return frames;
}

/* The demo on one pair of drivers: the one-page one (its address), the
 * two-page one (by name, as cc65's tgi_load_driver). 0: failed. */
static unsigned char demo(const void *one, const char *two) {
    unsigned int frames;
    unsigned char ok, err;
    tgi_install(one);
    err = tgi_geterror();
    if (err != TGI_ERR_OK) {
        printf("tgi_install: %s\n", tgi_geterrormsg(err));
        return 0;
    }
    wide = tgi_getmaxx() > 319;
    tgi_init();
    showcase();
#ifdef TGI_CHECK
    ok = tgi_getpixel(X(60), 110) == TGI_COLOR_YELLOW && tgi_getpixel(0, 0) == TGI_COLOR_WHITE;
#else
    ok = 1;
    while (!kbhit()) ;
    getch();
#endif
    tgi_unload();
    tgi_load_driver(two);
    if (tgi_geterror() != TGI_ERR_OK) {
        printf("%s: no room for two pages\n", two);
        return 0;
    }
    tgi_init();
    ok = ok && tgi_getpagecount() == 2;
#ifdef TGI_CHECK
    frames = animate(50);
    ok = ok && frames == 50;
#else
    frames = animate(0);
    (void)frames;
    getch();
#endif
    tgi_unload();
    return ok;
}

int main(void) {
    unsigned char ok = 1;
#ifndef TGI_640_ONLY
    ok = demo(tgi_static_stddrv, "mega65-fcm2.tgi");
#endif
    ok = ok && demo(tgi_static_ncm640, "mega65-ncm2.tgi");
#ifdef TGI_CHECK
    if (ok)
        printf("tgi check ok, 50 frames\n");
    else
        printf("tgi check failed\n");
#else
    (void)ok;
#endif
    return 0;
}
