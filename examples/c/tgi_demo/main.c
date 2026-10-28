/* tgi_demo — cc65's TGI (<tgi.h>) on the MEGA65
 *
 * Written as a cc65 TGI program is: the driver installed, tgi_init, and
 * only tgi_* calls after that, so it ports to and from the other TGI
 * platforms with its calls unchanged.
 *
 * First the drawing functions on the standard driver (320x200, 256
 * colours): the palette, lines clipped at the screen's edge, circle,
 * ellipse, arc and pie slice, bitmap text magnified and turned. A key goes
 * on to a star turning on the two-page driver: each frame is drawn on the
 * page not shown, then shown (tgi_setdrawpage / tgi_setviewpage), so no
 * frame is seen half drawn. A key ends it; tgi_unload puts the text screen
 * back.
 *
 * TGI_CHECK (make check) runs without keys: 50 frames, then reports on the
 * text screen.
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

static void wait_frame(void) {
    while (REG(0xD012) != 0xF8) ;
    while (REG(0xD012) == 0xF8) ;
}

static void showcase(void) {
    unsigned int i;
    unsigned int maxx = tgi_getmaxx(), maxy = tgi_getmaxy();
    tgi_clear();
    /* the first 32 palette entries */
    for (i = 0; i < 32; i++) {
        tgi_setcolor((unsigned char)i);
        tgi_bar(192 + (i & 15) * 8, 2 + (i >> 4) * 8, 199 + (i & 15) * 8, 9 + (i >> 4) * 8);
    }
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_line(0, 0, maxx, 0);
    tgi_lineto(maxx, maxy);
    tgi_lineto(0, maxy);
    tgi_lineto(0, 0);
    /* a fan of lines, clipped at the top */
    for (i = 0; i <= 15; i++) {
        tgi_setcolor((unsigned char)(2 + i % 14));
        tgi_line(60, 60, 60 + tgi_imulround(90, (signed int)(i * 24) - 180), -40);
    }
    tgi_setcolor(TGI_COLOR_YELLOW);
    tgi_circle(60, 150, 40);
    tgi_setcolor(TGI_COLOR_LIGHTGREEN);
    tgi_ellipse(140, 150, 40, 20);
    tgi_setcolor(TGI_COLOR_CYAN);
    tgi_pieslice(250, 155, 40, 30, 30, 120);
    tgi_setcolor(TGI_COLOR_LIGHTRED);
    tgi_arc(250, 155, 40, 30, 180, 330);
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_outtextxy(8, 78, "TGI on the MEGA65");
    tgi_settextscale(0x200, 0x200);
    tgi_setcolor(TGI_COLOR_ORANGE);
    tgi_outtextxy(8, 90, "320x200");
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_VERTICAL, TGI_FONT_BITMAP);
    tgi_setcolor(TGI_COLOR_LIGHTBLUE);
    tgi_outtextxy(300, 190, "256 colours");
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
    tgi_setcolor(TGI_COLOR_GRAY3);
    tgi_outtextxy(110, 188, "Press a key");
}

/* A five-pointed star turned by a (of 64): 10 corners, points and
 * notches */
static void star(unsigned char a, unsigned char colour) {
    unsigned char k, p;
    int x, y;
    tgi_setcolor(colour);
    for (k = 0; k <= 10; k++) {
        p = (unsigned char)((a + k * 32u / 5u) & 63);
        x = 160 + tgi_imulround((k & 1) ? 40 : 90, cos64[p]);
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

int main(void) {
    unsigned int frames;
    unsigned char err, pages;
    tgi_install(tgi_static_stddrv);
    err = tgi_geterror();
    if (err != TGI_ERR_OK) {
        printf("tgi_install: %s\n", tgi_geterrormsg(err));
        return 1;
    }
    tgi_init();
    showcase();
#ifdef TGI_CHECK
    err = tgi_getpixel(60, 110) == TGI_COLOR_YELLOW && tgi_getpixel(0, 0) == TGI_COLOR_WHITE;
#else
    while (!kbhit()) ;
    getch();
#endif
    tgi_unload();
    tgi_load_driver("mega65-fcm2.tgi");
    if (tgi_geterror() != TGI_ERR_OK) {
        printf("mega65-fcm2.tgi: no room for two pages\n");
        return 1;
    }
    tgi_init();
    pages = (unsigned char)tgi_getpagecount();
#ifdef TGI_CHECK
    frames = animate(50);
#else
    frames = animate(0);
    getch();
#endif
    tgi_unload();
#ifdef TGI_CHECK
    if (err && pages == 2 && frames == 50)
        printf("tgi check ok, %u frames\n", frames);
    else
        printf("tgi check failed\n");
#else
    (void)frames;
    (void)pages;
#endif
    return 0;
}
