/* test_float_conv_xemu.c — float/integer conversions and related fixes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] float -> int truncates toward zero (-2.5 -> -2, 2.9 -> 2)
 *   [1] float -> long beyond 16 bits (40000.0, -100000.0)
 *   [2] unsigned int 50000 / long 100000 -> float
 *   [3] -0.0 keeps its sign: signbit, copysign, runtime negation of 0
 *   [4] floor / ceil / trunc (incl. 1024 - 2^-15, |x| > 32767, negatives)
 *   [5] float ternary result (all 5 bytes)
 *   [6] float global initialized from a constant expression / integer
 *   [7] va_arg after a double named parameter; 5L varargs are longs
 *   [8] $AA sentinel
 */
#include <stdarg.h>
#define OUT ((volatile unsigned char *)0xC0C0)

extern double floor(double);
extern double ceil(double);
extern double trunc(double);
extern double copysign(double, double);

volatile double vneg = -2.5, vpos = 2.9, v40k = 40000.0, vm100k = -100000.0, vzero = 0.0;
double gexpr = 1024.0 - 1.0 / 32768.0;
float gint = 10;

double pick(double x) { return __builtin_signbit(x) ? 3.1415926535897932L : 0.5; }

double after_double(double one, ...) {
    va_list ap; double v;
    va_start(ap, one);
    v = va_arg(ap, double);
    va_end(ap);
    return one + v;
}

long sum_longs(int n, ...) {
    va_list ap; long s = 0;
    va_start(ap, n);
    while (n-- > 0) s += va_arg(ap, long);
    va_end(ap);
    return s;
}

int main(void) {
    unsigned int u = 50000;
    long l = 100000L;
    double n;
    OUT[0] = (int)vneg == -2 && (int)vpos == 2;
    OUT[1] = (long)v40k == 40000L && (long)vm100k == -100000L;
    OUT[2] = (double)u == 50000.0 && (double)l == 100000.0;
    n = -vzero;
    OUT[3] = __builtin_signbit(n) != 0 && copysign(1.0, n) < 0.0 && !__builtin_signbit(vzero);
    OUT[4] = floor(gexpr) == 1023.0 && floor(vneg) == -3.0 && ceil(vneg) == -2.0 &&
             trunc(vneg) == -2.0 && floor(v40k + 0.5) == 40000.0;
    OUT[5] = pick(-1.0) == 3.1415926535897932L && pick(1.0) == 0.5;
    OUT[6] = gexpr > 1023.99 && gexpr < 1024.0 && gint == 10.0f;
    OUT[7] = after_double(1.0, 2.0) == 3.0 && sum_longs(3, 1L, 2L, 70000L) == 70003L;
    OUT[8] = 0xAA;
    return 0;
}
