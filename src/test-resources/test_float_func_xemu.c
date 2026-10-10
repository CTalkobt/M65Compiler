/* test_float_func_xemu.c — float arguments/results (hidden result pointer),
 * negative values, and the ROM-backed math functions.
 *
 * Expected at $C0C0: 0F 01 01 01 06 07 04 00 0A 1B 03 20 3A 0F F9 AA
 */
#include <math.h>
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
float half(float x) { return x * 0.5; }
float add3(float a, float b, float c) { return a + b + c; }
float neg(float x) { return -x; }
float a = 3.0, b = 5.0, g = 16.0, m = -7.0;
void main() {
    float (*pg)(float) = half;
    float h = half(30.0);
    R(0) = (int)h;                          /* 15 */
    R(1) = (half(3.0) > 1.4 && half(3.0) < 1.6);
    R(2) = (add3(1.0, 2.0, 4.0) == 7.0);
    float d = a - b;                        /* -2 */
    R(3) = d < 0.0;
    R(4) = (int)(d * -3.0);                 /* 6 */
    R(5) = (int)fabsf(neg(7.0));            /* 7 */
    R(6) = (int)(sqrtf(g) + 0.5);           /* 4 */
    R(7) = (int)(sinf(0.0) * 10.0 + 0.5);   /* 0 */
    R(8) = (int)(cosf(0.0) * 10.0 + 0.5);   /* 10 */
    R(9) = (int)(expf(1.0) * 10.0);         /* 27 */
    R(10) = (int)(logf(expf(3.0)) + 0.5);   /* 3 */
    R(11) = (int)(powf(2.0, 5.0) + 0.5);    /* 32 */
    R(12) = (int)(atanf(1.0) * 400.0 + 0.5);/* 314 -> $3A */
    R(13) = (int)pg(30.0);                  /* 15 via function pointer */
    R(14) = (int)m;                         /* -7 from a negative initializer */
    R(15) = 0xAA;
}
