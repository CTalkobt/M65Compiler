/* tgi_arc.c — <tgi.h>'s arcs, pie slices, ellipses and circles
 *
 * An ellipse is Kennedy's midpoint ellipse ("A Fast Bresenham Type
 * Algorithm For Drawing Ellipses"): stdlib_asm/tgi_pixel.s45's
 * __tgi_ellipse walks a quarter in two passes, from (rx, 0) while the slope
 * is steeper than -1, then from (0, ry), with 32-bit adds only, and plots
 * each point in the four quarters; tgi_ellipse and tgi_circle are there too
 * (nothing here is linked for them). An arc is the ellipse's points between its angles: a point (x, y)
 * of the first quarter is at the angle t of (rx cos t, ry sin t), and
 *   c = y * rx * cos b - x * ry * sin b
 * is >= 0 when t >= b (b in 0..90); as the walk moves x or y by one, c
 * changes by one of the two products, so __tgi_ellipse keeps c for the
 * arc's start and end angles by adds too. Each quarter is drawn whole, not
 * at all, or where its one or two tests hold (the angle in a quarter maps to
 * t: 180 - t, 180 + t, 360 - t). Sines are an 8.8 table, needed only for
 * the two angles. An arc of an ellipse with a 0 radius is cc65's: lines
 * between points 12 degrees apart. Y grows downwards, so angles go
 * anticlockwise on the screen.
 */
#include <tgi.h>
#include <tgi/tgi-kernel.h>

/* round(256 * sin(d)) for d = 0..86 (87..90 are 256) */
static const unsigned char sintab[87] = {
    0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62,
    66, 71, 75, 79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124,
    128, 132, 136, 139, 143, 147, 150, 154, 158, 161, 165, 168, 171, 175, 178,
    181, 184, 187, 190, 193, 196, 199, 202, 204, 207, 210, 212, 215, 217, 219,
    222, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242, 243, 245, 246,
    247, 248, 249, 250, 251, 252, 253, 254, 254, 255, 255, 255
};

/* sin(d) in 8.8 for d = 0..90 */
static int sin90(unsigned d) {
    return d >= 87 ? 256 : sintab[d];
}

/* sin(d) in 8.8, d in degrees (0..450) */
static int isin(unsigned d) {
    while (d >= 360) d -= 360;
    if (d <= 90) return sin90(d);
    if (d <= 180) return sin90(180 - d);
    if (d <= 270) return -sin90(d - 180);
    return -sin90(360 - d);
}

static int icos(unsigned d) {
    return isin(d + 90);
}

int tgi_imulround(int rhs, int lhs) {
    signed long p = (signed long)rhs * lhs;
    unsigned char neg = p < 0;
    if (neg) p = -p;
    p = (p + 128) >> 8;
    return (int)(neg ? -p : p);
}

/* cc65's arc: lines between points 12 degrees apart (24 when rx + ry < 25):
 * an ellipse with a 0 radius (a line) */
static void poly_arc(int x, int y, unsigned char rx, unsigned char ry, unsigned sa, unsigned ea) {
    int x1, y1, x2, y2;
    unsigned char inc, done = 0;
    inc = (unsigned)rx + ry >= 25 ? 12 : 24;
    x1 = x + tgi_imulround(rx, icos(sa));
    y1 = y - tgi_imulround(ry, isin(sa));
    do {
        sa += inc;
        if (sa >= ea) {
            sa = ea;
            done = 1;
        }
        x2 = x + tgi_imulround(rx, icos(sa));
        y2 = y - tgi_imulround(ry, isin(sa));
        tgi_line(x1, y1, x2, y2);
        x1 = x2;
        y1 = y2;
    } while (!done);
}

/* tgi_pixel.s45's ellipse and the arc's numbers it keeps */
extern signed long __tgi_ks1, __tgi_ks2, __tgi_ke1, __tgi_ke2; /* the start's and end's c per y, x */
extern signed long __tgi_cs1, __tgi_ce1, __tgi_cs2, __tgi_ce2; /* their c at each pass's start */
extern unsigned char __tgi_allow[4];   /* a quarter's points shown in the states */
extern unsigned char __tgi_arcon;      /* $80: keep c_s and c_e */
__fastcall__ void __tgi_ellipse(int x, int y, unsigned char rx, unsigned char ry);

/* An angle in quarter q as the first quarter's t */
static unsigned base(unsigned char q, unsigned a) {
    if (q == 0) return a;
    if (q == 1) return 180 - a;
    if (q == 2) return a - 180;
    return 360 - a;
}

/* c's numbers for the angle b (0..90): the test is c >= 0, or c <= 0 (neg:
 * c negated) */
static void angle_test(unsigned b, unsigned char rx, unsigned char ry, unsigned char neg,
                       signed long *k1, signed long *k2, signed long *c1, signed long *c2) {
    signed long a = (signed long)rx * icos(b), s = (signed long)ry * isin(b);
    if (neg) {
        a = -a;
        s = -s;
    }
    *k1 = a;                            /* c per y */
    *k2 = s;                            /* c per x (subtracted) */
    *c1 = -(signed long)rx * s;         /* at (rx, 0) */
    *c2 = (signed long)ry * a;          /* at (0, ry) */
}

void tgi_arc(int x, int y, unsigned char rx, unsigned char ry, unsigned sa, unsigned ea) {
    unsigned char q, m;
    unsigned lo;
    if (sa > ea || !tgi_drv) return;
    if (ea > 360) ea = 360;
    if (!rx || !ry) {
        poly_arc(x, y, rx, ry, sa, ea);
        return;
    }
    __tgi_arcon = 0;
    __tgi_ks1 = __tgi_ks2 = __tgi_cs1 = __tgi_cs2 = 0;
    __tgi_ke1 = __tgi_ke2 = __tgi_ce1 = __tgi_ce2 = 0;
    for (q = 0; q < 4; q++) {
        lo = 90u * q;
        m = 0;
        if (ea >= lo && sa <= lo + 90) {
            m = 0x0F;                   /* every state */
            if (sa > lo) {              /* the start in this quarter: angle >= sa */
                angle_test(base(q, sa), rx, ry, q & 1, &__tgi_ks1, &__tgi_ks2, &__tgi_cs1, &__tgi_cs2);
                __tgi_arcon = 0x80;
                m &= 0x05;              /* c_s >= 0 */
            }
            if (ea < lo + 90) {         /* the end in it: angle <= ea */
                angle_test(base(q, ea), rx, ry, !(q & 1), &__tgi_ke1, &__tgi_ke2, &__tgi_ce1, &__tgi_ce2);
                __tgi_arcon = 0x80;
                m &= 0x03;              /* c_e >= 0 */
            }
        }
        __tgi_allow[q] = m;
    }
    __tgi_ellipse(x, y, rx, ry);
}

void tgi_pieslice(int x, int y, unsigned char rx, unsigned char ry, unsigned sa, unsigned ea) {
    tgi_arc(x, y, rx, ry, sa, ea);
    tgi_line(x, y, x + tgi_imulround(rx, icos(sa)), y - tgi_imulround(ry, isin(sa)));
    tgi_line(x, y, x + tgi_imulround(rx, icos(ea)), y - tgi_imulround(ry, isin(ea)));
}
