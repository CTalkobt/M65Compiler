/* test_tgi_layout_xemu.c — <tgi.h>'s TGI_IOCTL_LAYOUT, TGI_PIXEL_ADDR /
 * TGI_PIXEL_NIBBLE and the column table tgi_col_lo / tgi_col_hi (#653)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the column table: c * 1600 for c = 0..39
 *   [1] 320x200: the layout (the draw page, 1600, 8, 8 pixels, 8 bits)
 *   [2] 320x200: tgi_setpixel's pixels at TGI_PIXEL_ADDR (each corner, column
 *       edges), and a byte written there read by tgi_getpixel
 *   [3] 640x200: the layout (16 pixels, 4 bits); pixels as nibbles
 *       (TGI_PIXEL_NIBBLE: even x the low one), both ways
 *   [4] two pages (320x200 and 640x200): the layout's page follows
 *       tgi_setdrawpage, the pixels on that page
 *   [5] no driver: TGI_ERR_INV_FUNC, the layout untouched
 *   [6] $AA sentinel
 */
#include <tgi.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static tgi_layout_t l;

/* the colour of pixel (x, y) read from the page through the layout */
static unsigned char peek(unsigned int x, unsigned char y) {
    const unsigned char __far *p = (const unsigned char __far *)TGI_PIXEL_ADDR(&l, x, y);
    if (l.bpp == 8) return *p;
    return (unsigned char)(TGI_PIXEL_NIBBLE(&l, x) ? *p >> 4 : *p & 15);
}

static void poke(unsigned int x, unsigned char y, unsigned char c) {
    unsigned char __far *p = (unsigned char __far *)TGI_PIXEL_ADDR(&l, x, y);
    if (l.bpp == 8) *p = c;
    else if (TGI_PIXEL_NIBBLE(&l, x)) *p = (unsigned char)((*p & 0x0F) | (c << 4));
    else *p = (unsigned char)((*p & 0xF0) | (c & 15));
}

/* pixels set with tgi_setpixel read through the layout, and pixels written
 * through it read with tgi_getpixel (colours modulo the driver's count) */
static unsigned char pixels(unsigned char mask) {
    static const unsigned int xs[] = {0, 1, 7, 8, 15, 16, 17, 158, 159, 160, 318, 319, 320, 621, 638, 639};
    static const unsigned char ys[] = {0, 1, 7, 8, 99, 198, 199};
    unsigned int maxx = tgi_getmaxx();
    unsigned char i, j, c = 1, ok = 1;
    for (i = 0; i < sizeof xs / sizeof xs[0]; i++) {
        if (xs[i] > maxx) continue;
        for (j = 0; j < sizeof ys / sizeof ys[0]; j++) {
            c = (unsigned char)((c + 3) & mask);
            if (!c) c = 1;
            tgi_setcolor(c);
            tgi_setpixel(xs[i], ys[j]);
            if (peek(xs[i], ys[j]) != c) ok = 0;
            c = (unsigned char)((c + 5) & mask);
            poke(xs[i], ys[j], c);
            if (tgi_getpixel(xs[i], ys[j]) != c) ok = 0;
        }
    }
    /* a neighbour of a nibble kept */
    tgi_setcolor(0); tgi_setpixel(40, 50); tgi_setpixel(41, 50);
    poke(41, 50, 9 & mask);
    if (tgi_getpixel(40, 50) != 0 || tgi_getpixel(41, 50) != (9 & mask)) ok = 0;
    return ok;
}

static unsigned char layout_is(unsigned char px, unsigned char bpp) {
    unsigned long d = 0;
    tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR, &d);
    return tgi_ioctl(TGI_IOCTL_LAYOUT, &l) == TGI_ERR_OK && l.page == d && d != 0 &&
           l.col_bytes == 1600 && l.row_bytes == 8 && l.col_px == px && l.bpp == bpp;
}

static unsigned char twopages(const void *drv, unsigned char px, unsigned char bpp, unsigned char mask) {
    unsigned long p0;
    unsigned char ok = tgi_install(drv) == TGI_ERR_OK;
    tgi_init();
    ok = ok && layout_is(px, bpp);
    p0 = l.page;
    tgi_setcolor(3 & mask); tgi_setpixel(33, 44);
    tgi_setdrawpage(1);
    ok = ok && layout_is(px, bpp) && l.page != p0 && peek(33, 44) == 0;
    poke(33, 44, 6 & mask);
    ok = ok && tgi_getpixel(33, 44) == (6 & mask) && pixels(mask);
    tgi_setdrawpage(0);
    ok = ok && layout_is(px, bpp) && l.page == p0 && peek(33, 44) == (3 & mask);
    tgi_uninstall();
    return ok;
}

int main(void) {
    unsigned char ok, c;

    ok = 1;
    for (c = 0; c < 40; c++)
        if ((tgi_col_lo[c] | (unsigned int)tgi_col_hi[c] << 8) != c * 1600u) ok = 0;
    OUT[0] = ok;

    ok = tgi_install(tgi_static_stddrv) == TGI_ERR_OK;
    tgi_init();
    OUT[1] = ok && layout_is(8, 8);
    OUT[2] = pixels(0xFF);
    tgi_uninstall();

    ok = tgi_install(tgi_static_ncm640) == TGI_ERR_OK;
    tgi_init();
    ok = ok && layout_is(16, 4) && pixels(15);
    tgi_uninstall();
    OUT[3] = ok;

    ok = twopages(tgi_static_fcm2, 8, 8, 0xFF);
    OUT[4] = ok && twopages(tgi_static_ncm2, 16, 4, 15);

    l.col_bytes = 1234;
    tgi_geterror();
    OUT[5] = tgi_ioctl(TGI_IOCTL_LAYOUT, &l) == TGI_ERR_INV_FUNC && tgi_geterror() == TGI_ERR_INV_FUNC &&
             l.col_bytes == 1234;

    OUT[6] = 0xAA;
    return 0;
}
