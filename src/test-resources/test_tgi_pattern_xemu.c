/* test_tgi_pattern_xemu.c — tgi_bar's fill patterns (tgi_pattern.s45), and
 * gets / gets_s, heapfree, ftoa
 *
 * Results at $C0C0 (01 = pass):
 *   [0] gets / gets_s from keys in the KERNAL's buffer (a line too long
 *       for gets_s: "" and NULL)
 *   [1] heapfree, ftoa
 *   [2] 320x200: a colour pattern over a bar with ragged edges (single
 *       pixel columns) and whole character columns, anchored to the
 *       screen; the pixels around it kept
 *   [3] 320x200: a transparent colour (DMA transparency), a mono pattern,
 *       a transparent mono pattern
 *   [4] 320x200: tgi_setsolid and tgi_setpattern(NULL): the colour again
 *   [5] 640x200: a colour pattern (nibbles) over single pixels, pixel
 *       pairs and whole character columns
 *   [6] 640x200: a transparent mono pattern (a pixel at a time)
 *   [7] $AA sentinel
 */
#pragma cc45 signed_default

#include <tgi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char pat[64], pat2[64];
static const unsigned char bits[8] = { 0xAA, 0x55, 0xF0, 0x0F, 0x81, 0x42, 0x24, 0x18 };

static void keys(const char *k) {
    unsigned char n = 0;
    while (k[n]) { REG(0x02B0 + n) = k[n]; n++; }
    REG(0x00D0) = n;
}

static int t_gets(void) {
    char b[12];
    int ok;
    keys("hello\n");
    ok = gets(b) == b && strcmp(b, "hello") == 0;
    keys("abcdef\n");
    ok = ok && gets_s(b, 4) == NULL && b[0] == 0;
    keys("xy\n");
    ok = ok && gets_s(b, 10) == b && strcmp(b, "xy") == 0;
    return ok && gets_s(NULL, 5) == NULL;
}

static int t_misc(void) {
    char s[16];
    int ok = heapfree() == _heapmemavail() && heapfree() > 0;
    ftoa(1.5, s);
    ok = ok && strcmp(s, "1.5") == 0;
    ftoa(-0.25, s);
    ok = ok && strcmp(s, "-0.25") == 0;
    ftoa(1e10, s);
    return ok && strcmp(s, "1e+10") == 0;
}

/* The pixels of (x1, y1)-(x2, y2) that are not: the pattern's colour
 * (mask & it), or keep where the pattern has the transparent colour t
 * (t < 0: none) */
static unsigned int wrong(int x1, int y1, int x2, int y2, const unsigned char *p, int t, unsigned char keep,
                          unsigned char mask) {
    int x, y;
    unsigned int n = 0;
    unsigned char want, c;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++) {
            c = p[(y & 7) * 8 + (x & 7)];
            want = (t >= 0 && c == (unsigned char)t) ? keep : (unsigned char)(c & mask);
            if (tgi_getpixel(x, y) != want) n++;
        }
    return n;
}

static unsigned int around(int x1, int y1, int x2, int y2, unsigned char c) {
    unsigned int n = 0;
    int i;
    for (i = x1 - 1; i <= x2 + 1; i++) n += (tgi_getpixel(i, y1 - 1) != c) + (tgi_getpixel(i, y2 + 1) != c);
    for (i = y1; i <= y2; i++) n += (tgi_getpixel(x1 - 1, i) != c) + (tgi_getpixel(x2 + 1, i) != c);
    return n;
}

static void mono(unsigned char *p, unsigned char fg, unsigned char bg) {
    unsigned char r, i;
    for (r = 0; r < 8; r++)
        for (i = 0; i < 8; i++) p[r * 8 + i] = (bits[r] & (0x80 >> i)) ? fg : bg;
}

int main(void) {
    unsigned char i;
    int ok;
    OUT[0] = t_gets();
    OUT[1] = t_misc();
    for (i = 0; i < 64; i++) pat[i] = (unsigned char)(((i >> 3) << 4) + (i & 7) + 1);
    for (i = 0; i < 64; i++) pat2[i] = ((i ^ (i >> 3)) & 1) ? 7 : (unsigned char)(100 + i);

    /* 320x200 */
    ok = tgi_install(tgi_static_stddrv) == TGI_ERR_OK;
    tgi_init();
    tgi_setcolor(200);
    tgi_bar(0, 0, 100, 60);
    tgi_setpattern(pat, TGI_TRANSPARENT);
    tgi_bar(3, 5, 40, 30);
    OUT[2] = ok && wrong(3, 5, 40, 30, pat, -1, 0, 0xFF) == 0 && around(3, 5, 40, 30, 200) == 0;
    tgi_setpattern(pat2, 7);
    tgi_bar(1, 33, 50, 50);                 /* over 200: the 7s keep it */
    ok = wrong(1, 33, 50, 50, pat2, 7, 200, 0xFF) == 0;
    tgi_setpattern_mono(bits, 3, 9);
    mono(pat2, 3, 9);
    tgi_bar(60, 2, 90, 20);
    ok = ok && wrong(60, 2, 90, 20, pat2, -1, 0, 0xFF) == 0;
    tgi_setpattern_mono(bits, 4, TGI_TRANSPARENT);
    mono(pat2, 4, 5);
    tgi_bar(60, 2, 90, 20);                 /* over the last: its clear pixels 9 kept */
    for (i = 0; i < 64; i++) if (pat2[i] == 5) pat2[i] = 9;
    OUT[3] = ok && wrong(60, 2, 90, 20, pat2, -1, 0, 0xFF) == 0;
    tgi_setsolid();
    tgi_setcolor(11);
    tgi_bar(3, 5, 40, 30);
    ok = tgi_getpixel(3, 5) == 11 && tgi_getpixel(40, 30) == 11 && tgi_getpixel(20, 17) == 11;
    tgi_setpattern(pat, TGI_TRANSPARENT);
    tgi_setpattern(NULL, TGI_TRANSPARENT);
    tgi_setcolor(12);
    tgi_bar(3, 5, 40, 30);
    OUT[4] = ok && tgi_getpixel(3, 5) == 12 && tgi_getpixel(39, 29) == 12;
    tgi_done();
    tgi_uninstall();

    /* 640x200 */
    ok = tgi_install(tgi_static_ncm640) == TGI_ERR_OK;
    tgi_init();
    tgi_setcolor(6);
    tgi_bar(0, 0, 120, 60);
    tgi_setpattern(pat, TGI_TRANSPARENT);
    tgi_bar(5, 3, 70, 20);
    OUT[5] = ok && wrong(5, 3, 70, 20, pat, -1, 0, 0x0F) == 0 && around(5, 3, 70, 20, 6) == 0;
    tgi_setpattern_mono(bits, 13, TGI_TRANSPARENT);
    mono(pat2, 13, 5);
    tgi_bar(3, 25, 40, 40);                 /* over 6 */
    OUT[6] = wrong(3, 25, 40, 40, pat2, 5, 6, 0x0F) == 0;
    tgi_done();
    tgi_uninstall();
    OUT[7] = 0xAA;
    return 0;
}
