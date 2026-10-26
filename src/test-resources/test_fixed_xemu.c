/* test_fixed_xemu.c — fixed-point types (#72): __fixed8_8 and __fixed16_16
 *
 * Results at $C0C0 (01 = pass):
 *   [0] constants scaled when compiled: a global, an array, a struct, a
 *       local (the integers: 1.5 is $0180, -2.25 in 16.16 $FFFDC000)
 *   [1] + - and comparisons (with int and float constants, an int variable)
 *   [2] 8.8 * and /: signs, truncation (1/3 is $0055), 0.5 * 0.5
 *   [3] 16.16 * and / (the hardware multiplier and divider): signs, 1/3,
 *       100.5 * 2
 *   [4] 8.8 and 16.16 mixed (the wider type), int with fixed
 *   [5] conversions: to int (toward 0: -1.25 is -1), to and from float at
 *       run time, between the two types, toInt / toFloat of <fixed.h>
 *   [6] functions: fixed parameters (an int argument converted), results,
 *       a pointer to one, compound assignment on array elements
 *   [7] printf with toFloat8_8 / toFloat16_16 ("%f", "%.2f", "%g")
 *   [8] a little motion: position += velocity, velocity += gravity, 20
 *       steps, the integer position each time
 *   [9] ?: with a fixed branch and a constant / int one: the fixed type
 *   [10] $AA sentinel
 */
#include <stdio.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define RAW16(x) (*(unsigned int *)&(x))
#define RAW32(x) (*(unsigned long *)&(x))

__fixed8_8 g = 1.5;
__fixed16_16 g16 = -2.25;
__fixed8_8 tab[3] = { 0.5, 2, -1.25 };
struct P { __fixed8_8 x, y; } pt = { 3.5, -0.75 };
char buf[40];

static __fixed8_8 half(__fixed8_8 v) { return v / 2; }
static __fixed16_16 sq(__fixed16_16 v) { return v * v; }
static void set(__fixed8_8 *p, __fixed8_8 v) { *p = v; }

int main(void) {
    __fixed8_8 a = 2.5, b = -1.25, c, d;
    __fixed16_16 x = 100.5, y, z;
    signed int i = 3;
    signed long l;
    float f;
    unsigned char ok, k;

    {
        __fixed8_8 lc = -0.5;
        OUT[0] = RAW16(g) == 0x0180 && RAW32(g16) == 0xFFFDC000UL && RAW16(tab[0]) == 0x0080 &&
                 RAW16(tab[2]) == 0xFEC0 && RAW16(pt.x) == 0x0380 && RAW16(pt.y) == 0xFF40 && RAW16(lc) == 0xFF80;
    }

    c = a + b;
    d = a - 3;
    OUT[1] = RAW16(c) == 0x0140 && RAW16(d) == 0xFF80 && a > b && b < 0 && a == 2.5 && a != 2 && a >= 2 &&
             b <= -1.25 && a < i && !(a > i) && c + i == 4.25;

    ok = 1;
    c = a * b;                  if (RAW16(c) != 0xFCE0) ok = 0;   /* -3.125 */
    c = a / b;                  if (RAW16(c) != 0xFE00) ok = 0;   /* -2 */
    c = (__fixed8_8)1 / 3;      if (RAW16(c) != 0x0055) ok = 0;   /* 0.33203 */
    c = (__fixed8_8)0.5 * 0.5;  if (RAW16(c) != 0x0040) ok = 0;
    c = (__fixed8_8)-0.5 * 0.5; if (RAW16(c) != 0xFFC0) ok = 0;
    c = (__fixed8_8)-0.00390625 * 0.5; if (RAW16(c) != 0x0000) ok = 0;   /* -1/512: toward 0 */
    y = (__fixed16_16)-0.0000152587890625 * 0.5; if (RAW32(y) != 0UL) ok = 0;
    OUT[2] = ok;

    ok = 1;
    y = x * 2;                  if (RAW32(y) != 0x00C90000UL) ok = 0;   /* 201 */
    y = (__fixed16_16)-7.5 / 2.5; if (RAW32(y) != 0xFFFD0000UL) ok = 0; /* -3 */
    y = (__fixed16_16)1 / 3;    if (RAW32(y) != 0x00005555UL) ok = 0;
    y = (__fixed16_16)1.5 * g16; if (RAW32(y) != 0xFFFCA000UL) ok = 0;  /* -3.375 */
    y = sq(-3);                 if (RAW32(y) != 0x00090000UL) ok = 0;
    OUT[3] = ok;

    z = g16 + g;                /* -2.25 + 1.5 in 16.16 */
    y = g * x;                  /* 150.75 */
    OUT[4] = RAW32(z) == 0xFFFF4000UL && RAW32(y) == 0x0096C000UL && g + i == 4.5 && i - g == 1.5;

    ok = 1;
    i = b;                      if (i != -1) ok = 0;
    i = a;                      if (i != 2) ok = 0;
    l = x;                      if (l != 100) ok = 0;
    f = b;                      if (f != -1.25f) ok = 0;
    f = 0.75f;
    c = f;                      if (RAW16(c) != 0x00C0) ok = 0;
    y = c;                      if (RAW32(y) != 0x0000C000UL) ok = 0;   /* 8.8 -> 16.16 */
    c = x;                      if (RAW16(c) != 0x6480) ok = 0;         /* 16.16 -> 8.8 */
    if (toInt8_8(b) != -1 || toInt16_16(g16) != -2 || toFloat8_8(a) != 2.5f || toFloat16_16(g16) != -2.25f) ok = 0;
    OUT[5] = ok;

    ok = 1;
    c = half(a);                if (RAW16(c) != 0x0140) ok = 0;
    c = half(3);                if (RAW16(c) != 0x0180) ok = 0;
    set(&d, 1.25);              if (RAW16(d) != 0x0140) ok = 0;
    for (k = 0; k < 3; k++) tab[k] += 0.5;
    tab[1] *= 2;
    if (tab[0] != 1 || tab[1] != 5 || tab[2] != -0.75) ok = 0;
    OUT[6] = ok;

    sprintf(buf, "%f|%.2f|%g", toFloat8_8(b), toFloat16_16(x), toFloat8_8(a));
    OUT[7] = strcmp(buf, "-1.250000|100.50|2.5") == 0;

    {
        /* a ball thrown up: y += vy, vy -= 0.25 each step */
        __fixed8_8 py = 0, vy = 3;
        signed int path[20], expect = 0;
        __fixed8_8 ev = 3;
        __fixed8_8 ey = 0;
        ok = 1;
        for (k = 0; k < 20; k++) {
            py += vy;
            vy -= 0.25;
            path[k] = py;
        }
        for (k = 0; k < 20; k++) {
            ey = ey + ev;
            ev = ev - (__fixed8_8)0.25;
            expect = toInt8_8(ey);
            if (path[k] != expect) ok = 0;
        }
        if (path[0] != 3 || path[11] != 19 || toInt8_8(py) != 12) ok = 0;
        OUT[8] = ok;
    }
    k = 1;
    c = k ? a : 0.5;
    d = k ? 0.25 : a;
    OUT[9] = RAW16(c) == 0x0280 && RAW16(d) == 0x0040 && (k ? x : 2) == 100.5 && (!k ? x : 2) == 2;
    OUT[10] = 0xAA;
    return 0;
}
