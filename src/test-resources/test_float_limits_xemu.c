/* test_float_limits_xemu.c — <float.h> constants in the CBM 40-bit format,
 * and complex * and / of two complex values, __real__ / __imag__
 *
 * Results at $C0C0 (01 = pass):
 *   [0] FLT_MAX, FLT_MIN, FLT_EPSILON stored bytes (exponent, sign/mantissa):
 *       FF 7F FF FF FF, 01 00 00 00 00, 62 00 00 00 00 (2^-31)
 *   [1] FLT_EPSILON: 1 + eps != 1, 1 + eps/4 == 1; FLT_MIN > 0, FLT_MIN / 4
 *       underflows to 0; 1e38 < FLT_MAX
 *   [2] the integer limits: FLT_MANT_DIG 32, FLT_DIG 9, FLT_MIN_EXP -127,
 *       FLT_MAX_EXP 127, FLT_MAX_10_EXP 38, DBL_* equal to FLT_*
 *   [3] (1+2i)(3+4i) = -5+10i, (-5+10i)/(3+4i) = 1+2i (float and int)
 *   [4] __real__ / __imag__ read and assigned
 *   [5] operators on a 4-byte complex int temporary (returned in registers):
 *       (a + b) == s, (a + b + a), __real__ (a + b), ip / ib == ia
 *   [6] $AA sentinel
 */
#include <float.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static float fmax = FLT_MAX, fmin = FLT_MIN, feps = FLT_EPSILON;

static int bytes(const void *p, unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3,
                 unsigned char b4) {
    const unsigned char *b = (const unsigned char *)p;
    return b[0] == b0 && b[1] == b1 && b[2] == b2 && b[3] == b3 && b[4] == b4;
}

static float add(float a, float b) { return a + b; }

int main(void) {
    volatile float one = 1.0f, quarter = 0.25f, v;
    _Complex float a = 1.0f + 2.0fi, b = 3.0f + 4.0fi, p, q;
    _Complex int ia = 1 + 2i, ib = 3 + 4i, ip;
    int ok;

    OUT[0] = bytes(&fmax, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF) && bytes(&fmin, 0x01, 0x00, 0x00, 0x00, 0x00) &&
             bytes(&feps, 0x62, 0x00, 0x00, 0x00, 0x00);

    v = feps;
    ok = add(one, v) != one;
    v = feps * quarter;
    ok = ok && add(one, v) == one;
    v = fmin;
    ok = ok && v > 0.0f;
    v = fmin * quarter;
    ok = ok && v == 0.0f;
    v = 1e38f;
    OUT[1] = ok && v < fmax;

    OUT[2] = FLT_MANT_DIG == 32 && FLT_DIG == 9 && FLT_MIN_EXP == -127 && FLT_MAX_EXP == 127 &&
             FLT_MAX_10_EXP == 38 && FLT_MIN_10_EXP == -38 && FLT_RADIX == 2 &&
             DBL_MANT_DIG == FLT_MANT_DIG && DBL_MAX_EXP == FLT_MAX_EXP;

    p = a * b;
    q = p / b;
    ip = ia * ib;
    OUT[3] = __real__ p == -5.0f && __imag__ p == 10.0f && __real__ q == 1.0f && __imag__ q == 2.0f &&
             __real__ ip == -5 && __imag__ ip == 10 && ip / ib == ia;

    __real__ p = 7.0f;
    __imag__ p = -3.0f;
    OUT[4] = __real__ p == 7.0f && __imag__ p == -3.0f && __real__ a == 1.0f && __imag__ b == 4.0f;

    {
        _Complex int s = 4 + 6i;
        OUT[5] = (ia + ib) == s && (ia + ib + ia) == s + ia && __real__ (ia + ib) == 4 &&
                 __imag__ (ia * ib) == 10 && ip / ib == ia;
    }

    OUT[6] = 0xAA;
    return 0;
}
