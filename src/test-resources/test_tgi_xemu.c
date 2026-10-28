/* test_tgi_xemu.c — <tgi.h> (cc65's TGI) on the VIC-IV full-colour screen
 *
 * (.vfn files: test_tgi_vfn_xemu.c.) Results at $C0C0 (01 = pass):
 *   [0] tgi_install / tgi_init: the driver's numbers, the VIC in CHR16 +
 *       FCLRHI with the page's screen, white, no error
 *   [1] pixels: tgi_setpixel / tgi_getpixel, the page's layout (column of
 *       characters after column), nothing outside the screen
 *   [2] lines: ends and every pixel of a slope, the cursor, tgi_lineto,
 *       clipping (in part, wholly outside)
 *   [3] tgi_bar (swapped corners, clipped), tgi_clear
 *   [4] tgi_imulround, tgi_circle / tgi_arc / tgi_pieslice points, arcs
 *       across quarters, an ellipse clipped, a 0 radius
 *   [5] bitmap text: sizes, magnified, the glyph's pixels, the cursor
 *       (horizontal, vertical)
 *   [6] a vector font in memory: its lines, widths, scaling, the cursor
 *   [7] errors: tgi_geterror clears, bad page, second install, unknown
 *       driver, tgi_ioctl, tgi_geterrormsg
 *   [8] the palette: default, tgi_setpalette maps entries, tgi_done puts
 *       the palette back
 *   [9] tgi_done puts the text screen back; tgi_uninstall frees the page
 *  [10] "mega65-fcm2.tgi" by name: two pages, drawn and shown apart
 *  [11] 01
 *  [12] $AA sentinel
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <far_ptr.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned long scrptr(void) {
    return (unsigned long)REG(0xD060) | ((unsigned long)REG(0xD061) << 8) |
           ((unsigned long)REG(0xD062) << 16) | ((unsigned long)(REG(0xD063) & 0x0F) << 24);
}

static unsigned long draw_addr(void) {
    unsigned long a = 0;
    tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &a);
    return a;
}

/* pixel (x, y)'s byte in the draw page, read directly */
static unsigned char raw(signed int x, signed int y) {
    unsigned long a = draw_addr() + (unsigned long)((unsigned)x >> 3) * 1600u + (unsigned)y * 8u + ((unsigned)x & 7u);
    return *(unsigned char __far *)a;
}

/* pixels of colour c in the box */
static unsigned int count(signed int x1, signed int y1, signed int x2, signed int y2, unsigned char c) {
    signed int x, y;
    unsigned int n = 0;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            if (tgi_getpixel(x, y) == c) n++;
    return n;
}

/* A vector font: every character a move to (0,0) and a line to (4,8),
 * 6 wide */
static struct {
    unsigned char top, baseline, bottom;
    unsigned char widths[TGI_VF_CCOUNT];
    unsigned char *chars[TGI_VF_CCOUNT];
    unsigned char ops[4];
} vf;

static void make_font(void) {
    unsigned char i;
    vf.top = 8;
    vf.baseline = 0;
    vf.bottom = 2;
    vf.ops[0] = 0x00;           /* move to 0, 0 */
    vf.ops[1] = 0x00;
    vf.ops[2] = 0x84;           /* draw to 4, 8; the last */
    vf.ops[3] = 0x88;
    for (i = 0; i < TGI_VF_CCOUNT; i++) {
        vf.widths[i] = 6;
        vf.chars[i] = vf.ops;
    }
}

static unsigned char t_init(void) {
    if (tgi_install(tgi_static_stddrv) != TGI_ERR_OK) return 0;
    tgi_init();
    if (tgi_geterror() != TGI_ERR_OK) return 0;
    if (tgi_getmaxx() != 319 || tgi_getmaxy() != 199) return 0;
    if (tgi_getxres() != 320 || tgi_getyres() != 200) return 0;
    if (tgi_getcolorcount() != 0 || tgi_getmaxcolor() != 255) return 0;
    if (tgi_getpagecount() != 1 || tgi_getaspectratio() != 0xD4) return 0;
    if (tgi_getcolor() != TGI_COLOR_WHITE) return 0;
    if ((REG(0xD054) & 0x05) != 0x05 || REG(0xD058) != 80 || REG(0xD05E) != 40) return 0;
    if (REG(0xD031) & 0x88) return 0;
    if (draw_addr() < 0x40000UL || draw_addr() + 63999UL > 0x5FFFFUL || (draw_addr() & 63)) return 0;
    /* the screen's first cells: the page's first characters, down the column */
    {
        const unsigned char __far *s = (const unsigned char __far *)scrptr();
        unsigned int n = (unsigned int)(draw_addr() >> 6);
        if (s[0] != (unsigned char)n || s[1] != (unsigned char)(n >> 8)) return 0;
        if (s[80] != (unsigned char)(n + 1) || s[2] != (unsigned char)(n + 25)) return 0;
    }
    return 1;
}

static unsigned char t_pixels(void) {
    tgi_clear();
    tgi_setcolor(7);
    tgi_setpixel(0, 0);
    tgi_setcolor(200);
    tgi_setpixel(319, 199);
    tgi_setcolor(9);
    tgi_setpixel(13, 77);
    tgi_setpixel(-1, 5);
    tgi_setpixel(320, 5);
    tgi_setpixel(5, -1);
    tgi_setpixel(5, 200);
    if (tgi_getpixel(0, 0) != 7 || tgi_getpixel(319, 199) != 200 || tgi_getpixel(13, 77) != 9) return 0;
    if (raw(0, 0) != 7 || raw(319, 199) != 200 || raw(13, 77) != 9) return 0;
    if (tgi_getpixel(-1, 5) != 0 || tgi_getpixel(320, 5) != 0) return 0;
    if (count(0, 0, 319, 199, 0) != 64000u - 3u) return 0;
    return 1;
}

static unsigned char t_lines(void) {
    signed int i;
    tgi_clear();
    tgi_setcolor(3);
    tgi_line(10, 20, 100, 20);                  /* horizontal, across characters */
    if (count(0, 20, 319, 20, 3) != 91 || tgi_getpixel(9, 20) || tgi_getpixel(101, 20)) return 0;
    if (tgi_curx != 100 || tgi_cury != 20) return 0;
    tgi_lineto(100, 60);                        /* vertical */
    if (count(100, 0, 100, 199, 3) != 41) return 0;
    tgi_clear();
    tgi_line(50, 50, 10, 10);                   /* 45 degrees, backwards */
    for (i = 10; i <= 50; i++)
        if (tgi_getpixel(i, i) != 3) return 0;
    if (count(0, 0, 60, 60, 3) != 41) return 0;
    tgi_clear();
    tgi_line(0, 0, 20, 7);                      /* shallow: one pixel a column */
    for (i = 0; i <= 20; i++)
        if (count(i, 0, i, 10, 3) != 1) return 0;
    tgi_clear();
    tgi_line(-100, 100, 400, 100);              /* clipped at both ends */
    if (count(0, 100, 319, 100, 3) != 320) return 0;
    if (tgi_curx != 400) return 0;
    tgi_clear();
    tgi_line(-50, -10, 50, 90);                 /* clipped: enters at (0, 40) */
    if (tgi_getpixel(0, 40) != 3 || tgi_getpixel(50, 90) != 3 || tgi_getpixel(25, 65) != 3) return 0;
    if (count(0, 0, 319, 199, 3) != 51) return 0;
    tgi_clear();
    tgi_line(-10, -10, -1, 300);                /* wholly outside */
    tgi_line(330, 0, 400, 199);
    if (count(0, 0, 319, 199, 0) != 64000u) return 0;
    return 1;
}

static unsigned char t_bar(void) {
    tgi_clear();
    tgi_setcolor(5);
    tgi_bar(30, 40, 3, 10);                     /* swapped: 3..30 x 10..40 */
    if (count(0, 0, 40, 50, 5) != 28u * 31u) return 0;
    if (tgi_getpixel(2, 10) || tgi_getpixel(31, 10) || tgi_getpixel(3, 9) || tgi_getpixel(3, 41)) return 0;
    if (raw(16, 25) != 5) return 0;
    tgi_bar(310, 190, 400, 300);                /* clipped */
    if (count(300, 180, 319, 199, 5) != 100) return 0;
    tgi_bar(-5, -5, -1, 100);                   /* outside */
    if (count(0, 0, 319, 199, 5) != 28u * 31u + 100u) return 0;
    tgi_clear();
    if (count(0, 0, 319, 199, 0) != 64000u) return 0;
    return 1;
}

static unsigned char t_arcs(void) {
    if (tgi_imulround(100, 0x80) != 50 || tgi_imulround(-100, 0x80) != -50) return 0;
    if (tgi_imulround(3, 0x55) != 1 || tgi_imulround(50, 0xD4) != 41) return 0;
    tgi_clear();
    tgi_setcolor(4);
    tgi_circle(160, 100, 50);
    /* 0, 90, 180, 270 degrees: right, up, left, down */
    if (tgi_getpixel(210, 100) != 4 || tgi_getpixel(160, 50) != 4) return 0;
    if (tgi_getpixel(110, 100) != 4 || tgi_getpixel(160, 150) != 4) return 0;
    if (tgi_getpixel(160, 100) != 0) return 0;
    tgi_clear();
    tgi_arc(160, 100, 40, 20, 0, 90);           /* the top right quarter only */
    if (tgi_getpixel(200, 100) != 4 || tgi_getpixel(160, 80) != 4) return 0;
    if (count(100, 60, 159, 140, 4) != 0 || count(160, 101, 220, 140, 4) != 0) return 0;
    tgi_clear();
    tgi_pieslice(100, 100, 30, 30, 0, 90);      /* with its two radii */
    if (count(100, 100, 130, 100, 4) != 31 || count(100, 70, 100, 100, 4) != 31) return 0;
    tgi_arc(10, 10, 5, 5, 90, 0);               /* sa > ea: nothing */
    if (count(0, 0, 20, 20, 4) != 0) return 0;
    tgi_clear();
    tgi_arc(160, 100, 50, 50, 45, 135);         /* the top quarter, across two quarters */
    if (tgi_getpixel(160, 50) != 4 || tgi_getpixel(210, 100) || tgi_getpixel(110, 100)) return 0;
    if (count(100, 101, 220, 160, 4) != 0 || count(100, 50, 220, 64, 4) == 0) return 0;
    tgi_clear();
    tgi_arc(160, 100, 40, 40, 200, 340);        /* the bottom, not its ends' sides */
    if (tgi_getpixel(160, 140) != 4 || count(100, 60, 220, 100, 4) != 0) return 0;
    tgi_clear();
    tgi_ellipse(0, 100, 30, 20);                /* half off the screen: clipped */
    if (tgi_getpixel(30, 100) != 4 || tgi_getpixel(0, 80) != 4 || tgi_getpixel(0, 120) != 4) return 0;
    tgi_clear();
    tgi_ellipse(160, 100, 0, 20);               /* a 0 radius: a line */
    if (count(160, 80, 160, 120, 4) != 41 || count(150, 80, 159, 120, 4) != 0) return 0;
    return 1;
}

static unsigned char t_bitmap_text(void) {
    unsigned int n1, n2;
    if (tgi_gettextwidth("ABC") != 24 || tgi_gettextheight("ABC") != 8) return 0;
    tgi_clear();
    tgi_setcolor(6);
    tgi_outtextxy(16, 16, "H");
    /* 'H' in the lower/upper case font: rows $66 x3, $7E, $66 x3, $00 */
    if (tgi_getpixel(17, 16) != 6 || tgi_getpixel(18, 16) != 6 || tgi_getpixel(16, 16) != 0) return 0;
    if (tgi_getpixel(19, 19) != 6 || tgi_getpixel(19, 18) != 0 || count(16, 23, 23, 23, 6) != 0) return 0;
    n1 = count(16, 16, 23, 23, 6);
    if (n1 != 30 || tgi_curx != 24 || tgi_cury != 16) return 0;
    tgi_settextscale(0x200, 0x300);
    if (tgi_gettextwidth("AB") != 32 || tgi_gettextheight("AB") != 24) return 0;
    tgi_outtextxy(100, 100, "H");
    n2 = count(100, 100, 115, 123, 6);
    if (n2 != n1 * 6u || tgi_getpixel(102, 100) != 6 || tgi_getpixel(103, 102) != 6) return 0;
    if (tgi_curx != 116) return 0;
    tgi_settextscale(0x180, 0x100);             /* 1.5: rounded to 2 */
    if (tgi_gettextwidth("A") != 16) return 0;
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_VERTICAL, TGI_FONT_BITMAP);
    tgi_clear();
    tgi_outtextxy(50, 150, "HH");
    /* turned anticlockwise: the glyph's top row at the left, going up */
    if (count(50, 135, 57, 150, 6) != 2u * n1 || tgi_curx != 50 || tgi_cury != 134) return 0;
    if (tgi_getpixel(50, 149) != 6 || tgi_getpixel(50, 150) != 0) return 0;
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
    return 1;
}

static unsigned char t_vector_text(void) {
    make_font();
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_VECTOR);
    if (tgi_gettextwidth("AB") != 0) return 0;  /* no font yet */
    tgi_install_vectorfont((const tgi_vectorfont *)&vf);
    if (tgi_gettextwidth("AB") != 12 || tgi_gettextheight("AB") != 8) return 0;
    tgi_clear();
    tgi_setcolor(8);
    tgi_outtextxy(40, 100, "AB");
    /* each a line from the cursor to 4 right, 8 up */
    if (tgi_getpixel(40, 100) != 8 || tgi_getpixel(44, 92) != 8) return 0;
    if (tgi_getpixel(46, 100) != 8 || tgi_getpixel(50, 92) != 8) return 0;
    if (count(30, 80, 60, 110, 8) != 18 || tgi_curx != 52 || tgi_cury != 100) return 0;
    tgi_settextscale(0x200, 0x200);
    if (tgi_gettextwidth("AB") != 24 || tgi_gettextheight("AB") != 16) return 0;
    tgi_clear();
    tgi_outtextxy(40, 100, "A");
    if (tgi_getpixel(48, 84) != 8 || tgi_curx != 52) return 0;
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_VERTICAL, TGI_FONT_VECTOR);
    tgi_clear();
    tgi_outtextxy(100, 100, "A");               /* 4 up, 8 left */
    if (tgi_getpixel(92, 96) != 8 || tgi_cury != 94 || tgi_curx != 100) return 0;
    tgi_install_vectorfont(0);
    tgi_settextstyle(0x100, 0x100, TGI_TEXT_HORIZONTAL, TGI_FONT_BITMAP);
    return 1;
}

static unsigned char t_errors(void) {
    unsigned long a;
    if (tgi_geterror() != TGI_ERR_OK) return 0;
    tgi_setdrawpage(1);
    if (tgi_geterror() != TGI_ERR_INV_ARG || tgi_geterror() != TGI_ERR_OK) return 0;
    tgi_setviewpage(2);
    if (tgi_geterror() != TGI_ERR_INV_ARG) return 0;
    if (tgi_install(tgi_static_stddrv) != TGI_ERR_INSTALLED) return 0;
    tgi_geterror();
    tgi_load_driver(tgi_stddrv);
    if (tgi_geterror() != TGI_ERR_INSTALLED) return 0;
    if (tgi_ioctl(0x12, &a) != TGI_ERR_INV_FUNC || tgi_geterror() != TGI_ERR_INV_FUNC) return 0;
    if (strcmp(tgi_geterrormsg(TGI_ERR_OK), "No error")) return 0;
    if (strcmp(tgi_geterrormsg(TGI_ERR_INSTALLED), "A driver is already installed")) return 0;
    if (strcmp(tgi_geterrormsg(200), "Unknown error")) return 0;
    return 1;
}

static unsigned char saved[3][4];

static unsigned char t_palette(void) {
    static unsigned char p[256];
    const unsigned char *d = tgi_getdefpalette();
    unsigned int i;
    unsigned char k;
    for (i = 0; i < 256; i++)
        if (d[i] != (unsigned char)i) return 0;
    for (k = 0; k < 3; k++)
        for (i = 0; i < 4; i++) saved[k][i] = REG(0xD100 + ((unsigned int)k << 8) + i);
    memcpy(p, d, 256);
    p[0] = 2;                                   /* entry 0 shows red, 1 black */
    p[1] = 0;
    tgi_setpalette(p);
    if (tgi_getpalette()[0] != 2 || tgi_getpalette()[1] != 0 || tgi_getpalette()[3] != 3) return 0;
    for (k = 0; k < 3; k++) {
        if (REG(0xD100 + ((unsigned int)k << 8)) != saved[k][2]) return 0;
        if (REG(0xD101 + ((unsigned int)k << 8)) != saved[k][0]) return 0;
        if (REG(0xD103 + ((unsigned int)k << 8)) != saved[k][3]) return 0;
    }
    return 1;
}

static unsigned char t_done(unsigned char d054, unsigned char d031, unsigned long scr) {
    unsigned char k, i;
    far_ptr_t m;
    tgi_done();
    if (REG(0xD054) != d054 || REG(0xD031) != d031 || scrptr() != scr || REG(0xD058) != 80) return 0;
    for (k = 0; k < 3; k++)
        for (i = 0; i < 4; i++)
            if (REG(0xD100 + ((unsigned int)k << 8) + i) != saved[k][i]) return 0;
    tgi_done();                                 /* again: nothing */
    tgi_uninstall();
    if (tgi_drv || tgi_geterror() != TGI_ERR_OK) return 0;
    m = far_malloc(64000u, BANK4);
    if (far_is_null(m)) return 0;
    far_free(m);
    tgi_unload();                               /* no driver */
    if (tgi_geterror() != TGI_ERR_NO_DRIVER) return 0;
    tgi_init();
    if (tgi_geterror() != TGI_ERR_NO_DRIVER) return 0;
    return 1;
}

static unsigned char t_two_pages(void) {
    unsigned long p0, p1, s0;
    tgi_load_driver("MEGA65-FCM2.TGI");
    if (tgi_geterror() != TGI_ERR_OK || tgi_getpagecount() != 2) return 0;
    tgi_init();
    p0 = draw_addr();
    s0 = scrptr();
    tgi_setdrawpage(1);
    p1 = draw_addr();
    if (p0 == p1 || (p0 >> 16) == (p1 >> 16)) return 0;
    tgi_clear();
    tgi_setcolor(11);
    tgi_setpixel(5, 5);
    if (tgi_getpixel(5, 5) != 11) return 0;
    tgi_setdrawpage(0);
    tgi_clear();
    if (tgi_getpixel(5, 5) != 0) return 0;
    tgi_setviewpage(1);
    if (scrptr() == s0) return 0;
    {
        const unsigned char __far *s = (const unsigned char __far *)scrptr();
        unsigned int n = (unsigned int)(p1 >> 6);
        if (s[0] != (unsigned char)n || s[1] != (unsigned char)(n >> 8)) return 0;
    }
    tgi_setviewpage(0);
    if (scrptr() != s0) return 0;
    tgi_unload();
    return tgi_drv == 0 && tgi_gmode == 0;
}

int main(void) {
    unsigned char d054 = REG(0xD054), d031 = REG(0xD031);
    unsigned long scr = scrptr();
    unsigned char i;
    for (i = 0; i < 12; i++) OUT[i] = 0;
    OUT[12] = 0xAA;
    if (!t_init()) return 1;
    OUT[0] = 1;
    OUT[1] = t_pixels();
    OUT[2] = t_lines();
    OUT[3] = t_bar();
    OUT[4] = t_arcs();
    OUT[5] = t_bitmap_text();
    OUT[6] = t_vector_text();
    OUT[7] = t_errors();
    OUT[8] = t_palette();
    OUT[9] = t_done(d054, d031, scr);
    OUT[10] = t_two_pages();
    OUT[11] = 1;
    return 0;
}
