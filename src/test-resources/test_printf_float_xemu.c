/* test_printf_float_xemu.c — printf/sprintf %f %e %g (and %F %E %G) over the
 * whole CBM float range (digits from the BASIC 65 ROM's number formatting)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] %f: default precision, large integer parts (no 16-bit limit), .N
 *   [1] %e / %E: mantissa, exponent sign and width, rounding into the exponent
 *   [2] %g / %G: fixed or exponent form, trailing zeros, # flag
 *   [3] rounding half up, carries (9.9996 -> 10.000), precision 0, # flag
 *   [4] zero, negative values, tiny values, flags + space and width/-/0
 *   [5] the largest float, precision beyond the 9-10 digits the format holds,
 *       1e20 (FOUT prints a value just below it as " 0E19")
 *   [6] printf to the screen returns the same count as sprintf
 *   [7] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <float.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[64];

static int is(const char *want, const char *fmt, double v) {
    int n = sprintf(buf, fmt, v);
    return n == (int)strlen(want) && strcmp(buf, want) == 0;
}

int main(void) {
    int ok;

    ok = is("3.250000", "%f", 3.25) && is("100000.000000", "%f", 100000.0) &&
         is("65536.00", "%.2f", 65536.0) && is("-1234.57", "%.2f", -1234.567) &&
         is("1000000000000000000000000000000.000000", "%f", 1.0e30) &&
         is("123456789.0", "%.1f", 123456789.0);
    OUT[0] = ok;

    ok = is("1.000000e+30", "%e", 1.0e30) && is("1.230000E-04", "%E", 0.000123) &&
         is("2.50e+00", "%.2e", 2.5) && is("1.0e+01", "%.1e", 9.96) &&
         is("1e-10", "%.0e", 1.0e-10) && is("-4.500e+20", "%.3e", -4.5e20);
    OUT[1] = ok;

    ok = is("0.000123", "%g", 0.000123) && is("1.23457e+06", "%g", 1234567.0) &&
         is("100", "%g", 100.0) && is("3.14", "%.3g", 3.14159) &&
         is("1.5E-10", "%G", 1.5e-10) && is("1.00000", "%#g", 1.0) &&
         is("1e+20", "%g", 1.0e20) && is("0.0001", "%g", 0.0001) &&
         is("1.000000e+20", "%e", 99999999999999999999.0);
    OUT[2] = ok;

    ok = is("10.000", "%.3f", 9.9996) && is("0.001", "%.3f", 0.0005) &&
         is("3", "%.0f", 2.5) && is("3.", "%#.0f", 3.0) && is("100", "%.0f", 99.5) &&
         is("1.0", "%.1f", 0.95);
    OUT[3] = ok;

    ok = is("0.000000", "%f", 0.0) && is("0.000000e+00", "%e", 0.0) && is("0", "%g", 0.0) &&
         is("-0.5", "%g", -0.5) && is("0.000000", "%f", 1.0e-7) &&
         is("+2.0", "%+.1f", 2.0) && is(" 2.0", "% .1f", 2.0) &&
         is("   2.500", "%8.3f", 2.5) && is("2.5     |", "%-8.1f|", 2.5) &&
         is("-002.50", "%07.2f", -2.5);
    OUT[4] = ok;

    /* (FLT_MAX is the constant 1.7014118e38; the format holds 9-10 digits) */
    ok = is("1.70141e+38", "%g", FLT_MAX) && is("1.701411800e+38", "%.9e", FLT_MAX) &&
         is("0.50000000000000000000", "%.20f", 0.5) &&
         sprintf(buf, "%f", FLT_MAX) == 46;
    {
        /* one unit below 1e20: BASIC's FOUT gives " 0E19" for it */
        union { float f; unsigned char b[5]; } u;
        u.b[0] = 0xC3; u.b[1] = 0x2D; u.b[2] = 0x78; u.b[3] = 0xEB; u.b[4] = 0xC5;
        ok = ok && is("1.000000e+20", "%e", u.f) && is("1e+20", "%g", u.f);
    }
    OUT[5] = ok;

    {
        int a = sprintf(buf, "%g|%f", 1.25, -7.5);
        int b = printf("%g|%f\n", 1.25, -7.5);
        OUT[6] = a == 14 && b == 15;
    }
    OUT[7] = 0xAA;
    return 0;
}
