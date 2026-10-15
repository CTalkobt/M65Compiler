/* floatmath.c — Float math functions for cc45 / MEGA65
 *
 * Implemented in C using ROM-backed primitives (sinf, cosf, expf, logf, etc.)
 * and basic float arithmetic. All functions use CBM 40-bit float.
 */

float sinf(float x);
float cosf(float x);
float atanf(float x);
float logf(float x);
float expf(float x);
float sqrtf(float x);
float fabsf(float x);
float truncf(float x);
float frexpf(float x, signed int *exp);

#include <errno.h>

#define FLOAT_MAX 1.7014118342085515e38

/* powf: x^y = exp(y * log(|x|)). A negative base needs an integer exponent
 * (otherwise there is no value: 0, EDOM; an odd one keeps the sign);
 * 0 to a negative power is a pole: FLOAT_MAX, ERANGE. Overflow saturates
 * in expf (ERANGE). */
float powf(float x, float y) {
    float r;
    int odd = 0;
    if (y == 0.0) return 1.0;
    if (x == 0.0) {
        if (y < 0.0) { errno = ERANGE; return FLOAT_MAX; }
        return 0.0;
    }
    if (x < 0.0) {
        if (truncf(y) != y) { errno = EDOM; return 0.0; }
        /* |y| >= 2^31 is even (a float that large has no fraction bits) */
        if (fabsf(y) < 2147483648.0) odd = ((signed long)y & 1) != 0;
        x = 0.0 - x;
    }
    r = expf(y * logf(x));
    return odd ? 0.0 - r : r;
}

/* truncf: truncate toward zero. |x| >= 2^31 is already an integer (the
 * exponent byte is $A0 or more); smaller values go through long. The result
 * keeps x's sign (trunc(-0.5) is -0.0). */
float truncf(float x) {
    unsigned char *px = (unsigned char *)&x;
    float t;
    unsigned char *pt = (unsigned char *)&t;
    if (px[0] >= 0xA0) return x;
    t = (float)(signed long)x;
    pt[1] = (pt[1] & 0x7F) | (px[1] & 0x80);
    return t;
}

/* floorf: largest integer <= x */
float floorf(float x) {
    float t = truncf(x);
    if (x < 0.0 && t != x) t = t - 1.0;
    return t;
}

/* ceilf: smallest integer >= x */
float ceilf(float x) {
    float t = truncf(x);
    if (x > 0.0 && t != x) t = t + 1.0;
    return t;
}

/* roundf: round to nearest, ties away from zero */
float roundf(float x) {
    if (x >= 0.0) return floorf(x + 0.5);
    return ceilf(x - 0.5);
}

/* fmodf: floating-point remainder */
float fmodf(float x, float y) {
    if (y == 0.0) { errno = EDOM; return 0.0; }  /* no value (NaN in IEEE) */
    return x - truncf(x / y) * y;
}

/* modff: split into integer and fractional parts */
float modff(float x, float *iptr) {
    float i = truncf(x);
    *iptr = i;
    return x - i;
}

/* atan2f: two-argument arctangent using ROM atan
 * Returns angle in radians [-pi, pi] for point (x, y) */
float atan2f(float y, float x) {
    float pi = 3.14159265;
    if (x > 0.0) return atanf(y / x);
    if (x < 0.0) {
        if (y >= 0.0) return atanf(y / x) + pi;
        return atanf(y / x) - pi;
    }
    /* x == 0 */
    if (y > 0.0) return pi / 2.0;
    if (y < 0.0) return 0.0 - pi / 2.0;
    return 0.0;
}

/* log10f: base-10 logarithm = ln(x) / ln(10) */
float log10f(float x) {
    return logf(x) * 0.43429448;  /* 1/ln(10) */
}

/* log2f: base-2 logarithm, exact for powers of two: x = f * 2^e with f in
 * [0.5, 1), log2(x) = e + ln(f) / ln(2) */
float log2f(float x) {
    signed int e;
    float f;
    if (x <= 0.0) return logf(x);    /* errno and the result as logf */
    f = frexpf(x, &e);
    if (f == 0.5) return (float)(e - 1);
    return (float)e + logf(f) * 1.44269504;  /* 1/ln(2) */
}

/* The CBM float keeps the binary exponent in its first byte (bias 128, 0 for
 * zero): x = 0.1mmm... * 2^(byte0 - 128). ldexpf and frexpf change it. */

/* ldexpf: x * 2^n, exact; beyond the range ±largest float (ERANGE), below
 * it 0 (ERANGE) */
float ldexpf(float x, signed int n) {
    unsigned char *px = (unsigned char *)&x;
    signed int e;
    if (px[0] == 0 || n == 0) return x;
    e = (signed int)px[0] + n;
    if (e > 255) {
        errno = ERANGE;
        px[0] = 0xFF; px[1] |= 0x7F; px[2] = 0xFF; px[3] = 0xFF; px[4] = 0xFF;
        return x;
    }
    if (e < 1) {
        errno = ERANGE;
        return 0.0;
    }
    px[0] = (unsigned char)e;
    return x;
}

/* frexpf: x = frac * 2^exp with |frac| in [0.5, 1), exact */
float frexpf(float x, signed int *exp) {
    unsigned char *px = (unsigned char *)&x;
    if (px[0] == 0) { *exp = 0; return 0.0; }
    *exp = (signed int)px[0] - 128;
    px[0] = 128;
    return x;
}

/* copysignf: magnitude of x with sign of y */
float copysignf(float x, float y) {
    /* The sign bit (bit 7 of the first mantissa byte) of y onto x, so that
       copysignf(1, -0.0) is -1 */
    unsigned char *px = (unsigned char *)&x;
    px[1] = (px[1] & 0x7F) | (((unsigned char *)&y)[1] & 0x80);
    return x;
}

/* fmaxf / fminf / fdimf */
float fmaxf(float x, float y) { if (x >= y) return x; return y; }
float fminf(float x, float y) { if (x <= y) return x; return y; }
float fdimf(float x, float y) { if (x > y) return x - y; return 0.0; }

/* Classification helpers (CBM 40-bit has no NaN/Inf representation) */
int __isnanf(float x) { return 0; }
int __isinff(float x) { return 0; }
int __fpclassifyf(float x) { if (x == 0.0) return 0; return 1; }
