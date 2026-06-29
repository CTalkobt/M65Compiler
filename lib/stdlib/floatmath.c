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

/* powf: x^y = exp(y * log(x)) */
float powf(float x, float y) {
    if (y == 0.0) return 1.0;
    if (x == 0.0) return 0.0;
    if (x < 0.0) {
        /* Negative base: only works for integer exponents */
        int iy = (int)y;
        float r = expf((float)iy * logf(0.0 - x));
        if (iy % 2 != 0) r = 0.0 - r;
        return r;
    }
    return expf(y * logf(x));
}

/* truncf: truncate toward zero */
float truncf(float x) {
    return (float)((int)x);
}

/* floorf: largest integer <= x */
float floorf(float x) {
    float t = (float)((int)x);
    if (x < 0.0 && t != x) t = t - 1.0;
    return t;
}

/* ceilf: smallest integer >= x */
float ceilf(float x) {
    float t = (float)((int)x);
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
    if (y == 0.0) return 0.0;
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

/* log2f: base-2 logarithm = ln(x) / ln(2) */
float log2f(float x) {
    return logf(x) * 1.44269504;  /* 1/ln(2) */
}

/* ldexpf: x * 2^n */
float ldexpf(float x, int n) {
    float scale = 1.0;
    if (n > 0) {
        while (n > 0) { scale = scale * 2.0; n = n - 1; }
    } else {
        while (n < 0) { scale = scale * 0.5; n = n + 1; }
    }
    return x * scale;
}

/* frexpf: split x into normalized fraction [0.5, 1.0) and exponent
 * x = frac * 2^exp */
float frexpf(float x, int *exp) {
    if (x == 0.0) { *exp = 0; return 0.0; }
    int e = 0;
    float f = fabsf(x);
    while (f >= 1.0) { f = f * 0.5; e = e + 1; }
    while (f < 0.5) { f = f * 2.0; e = e - 1; }
    *exp = e;
    if (x < 0.0) f = 0.0 - f;
    return f;
}

/* copysignf: magnitude of x with sign of y */
float copysignf(float x, float y) {
    float ax = fabsf(x);
    if (y < 0.0) return 0.0 - ax;
    return ax;
}

/* fmaxf / fminf / fdimf */
float fmaxf(float x, float y) { if (x >= y) return x; return y; }
float fminf(float x, float y) { if (x <= y) return x; return y; }
float fdimf(float x, float y) { if (x > y) return x - y; return 0.0; }

/* Classification helpers (CBM 40-bit has no NaN/Inf representation) */
int __isnanf(float x) { return 0; }
int __isinff(float x) { return 0; }
int __fpclassifyf(float x) { if (x == 0.0) return 0; return 1; }
