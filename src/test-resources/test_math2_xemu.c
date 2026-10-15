/* test_math2_xemu.c — math.h functions not covered elsewhere
 *
 * Results at $C0C0 (01 = pass):
 *   [0] atan, atan2 / atan2f (all four quadrants and the axes)
 *   [1] fmod / fmodf (signs, fmod(x, 0) is EDOM)
 *   [2] frexp / frexpf, ldexp / ldexpf (exact; out of range is ERANGE)
 *   [3] modf / modff
 *   [4] log2 / log2f (exact for powers of two)
 *   [5] ceilf, roundf, truncf (signs, halves, large values)
 *   [6] copysignf, fdim / fdimf, fmaxf, fminf, signbitf
 *   [7] the double names (atan, fdim, fmod, frexp, ldexp, log2, modf) in
 *       math_double.c, called through function pointers
 *   [8] $AA sentinel
 */
#include <math.h>
#include <errno.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define PI 3.14159265

static int near(float a, float b) {
    float d = a - b;
    if (d < 0.0) d = 0.0 - d;
    float m = b < 0.0 ? 0.0 - b : b;
    return d <= 0.000001 * (m > 1.0 ? m : 1.0);
}

static int t_atan(void) {
    return near(atanf(1.0), PI / 4.0) && near(atanf(-1.0), 0.0 - PI / 4.0) && atanf(0.0) == 0.0 &&
           near(atan2f(1.0, 1.0), PI / 4.0) && near(atan2f(1.0, -1.0), 3.0 * PI / 4.0) &&
           near(atan2f(-1.0, -1.0), -3.0 * PI / 4.0) && near(atan2f(-1.0, 1.0), 0.0 - PI / 4.0) &&
           near(atan2f(2.0, 0.0), PI / 2.0) && near(atan2f(-2.0, 0.0), 0.0 - PI / 2.0) &&
           near(atan2f(0.0, -3.0), PI) && atan2f(0.0, 3.0) == 0.0 && atan2f(0.0, 0.0) == 0.0;
}

static int t_fmod(void) {
    int ok = fmodf(7.0, 3.0) == 1.0 && fmodf(-7.0, 3.0) == -1.0 && fmodf(7.0, -3.0) == 1.0 &&
             near(fmodf(5.5, 2.0), 1.5) && fmodf(6.0, 3.0) == 0.0 && near(fmodf(100000.5, 7.0), 5.5);
    errno = 0;
    ok = ok && fmodf(1.0, 0.0) == 0.0 && errno == EDOM;
    return ok;
}

static int t_frexp(void) {
    signed int e;
    float f;
    int ok = 1;
    f = frexpf(8.0, &e);   ok = ok && f == 0.5 && e == 4;
    f = frexpf(1.0, &e);   ok = ok && f == 0.5 && e == 1;
    f = frexpf(0.75, &e);  ok = ok && f == 0.75 && e == 0;
    f = frexpf(-3.0, &e);  ok = ok && f == -0.75 && e == 2;
    f = frexpf(0.0, &e);   ok = ok && f == 0.0 && e == 0;
    f = frexpf(0.001, &e); ok = ok && f >= 0.5 && f < 1.0 && e == -9 && ldexpf(f, e) == 0.001;
    ok = ok && ldexpf(0.75, 3) == 6.0 && ldexpf(-1.0, -2) == -0.25 && ldexpf(0.0, 50) == 0.0 &&
         ldexpf(3.0, 0) == 3.0 && ldexpf(1.0, 100) == 1267650600228229401496703205376.0;
    errno = 0;
    ok = ok && ldexpf(1.0, 200) > 1.0e38 && errno == ERANGE;
    errno = 0;
    ok = ok && ldexpf(1.0, -200) == 0.0 && errno == ERANGE;
    return ok;
}

static int t_modf(void) {
    float ip;
    float fr = modff(3.75, &ip);
    int ok = fr == 0.75 && ip == 3.0;
    fr = modff(-2.5, &ip);
    ok = ok && fr == -0.5 && ip == -2.0;
    fr = modff(7.0, &ip);
    return ok && fr == 0.0 && ip == 7.0;
}

static int t_log2(void) {
    return log2f(8.0) == 3.0 && log2f(1.0) == 0.0 && log2f(0.25) == -2.0 && log2f(1024.0) == 10.0 &&
           near(log2f(10.0), 3.32192809) && near(log2f(3.0), 1.5849625);
}

static int t_round(void) {
    return ceilf(1.2) == 2.0 && ceilf(-1.2) == -1.0 && ceilf(3.0) == 3.0 &&
           roundf(2.5) == 3.0 && roundf(-2.5) == -3.0 && roundf(2.49) == 2.0 && roundf(-0.4) == 0.0 &&
           truncf(2.9) == 2.0 && truncf(-2.9) == -2.0 && truncf(5000000000.5) == 5000000000.0 &&
           floorf(-0.5) == -1.0 && truncf(1.0e20) == 1.0e20;
}

static int t_misc(void) {
    return copysignf(3.0, -1.0) == -3.0 && copysignf(-3.0, 2.0) == 3.0 && copysignf(0.0, 1.0) == 0.0 &&
           fdimf(5.0, 3.0) == 2.0 && fdimf(3.0, 5.0) == 0.0 && fmaxf(-1.0, 2.0) == 2.0 &&
           fminf(-1.0, 2.0) == -1.0 && signbitf(-2.0) && !signbitf(2.0) && !signbitf(0.0);
}

/* The library's double functions (not the math.h macros) */
double (*p_atan)(double) = (atan);
double (*p_fdim)(double, double) = (fdim);
double (*p_fmod)(double, double) = (fmod);
double (*p_frexp)(double, signed int *) = (frexp);
double (*p_ldexp)(double, signed int) = (ldexp);
double (*p_log2)(double) = (log2);
double (*p_modf)(double, double *) = (modf);

static int t_double(void) {
    signed int e;
    double ip;
    return near(p_atan(1.0), PI / 4.0) && p_fdim(4.0, 1.0) == 3.0 && p_fmod(7.0, 3.0) == 1.0 &&
           p_frexp(8.0, &e) == 0.5 && e == 4 && p_ldexp(0.5, 4) == 8.0 && p_log2(16.0) == 4.0 &&
           p_modf(2.25, &ip) == 0.25 && ip == 2.0 && atan2(1.0, 0.0) > 1.57;
}

int main(void) {
    OUT[0] = t_atan();
    OUT[1] = t_fmod();
    OUT[2] = t_frexp();
    OUT[3] = t_modf();
    OUT[4] = t_log2();
    OUT[5] = t_round();
    OUT[6] = t_misc();
    OUT[7] = t_double();
    OUT[8] = 0xAA;
    return 0;
}
