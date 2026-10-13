/* test_signext_xemu.c — sign extension and related fixes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] signed long >> 8 / 16 / 24 keep the sign (positive and negative)
 *   [1] signed long / int shifted out (>= width): 0 or -1
 *   [2] int + long: the int is sign-extended before a 32-bit add
 *   [3] sprintf %f (float-aware sprintf linked)
 *   [4] va_list passed to an always_inline function (nested)
 *   [5] $AA sentinel
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

volatile signed long p = 0x12345678L, n = -0x12345678L;
volatile signed int pi = 0x1234, ni = -0x1234, zero = 0;
signed long x, y;

signed long add5(signed int i, signed long a, signed long b, signed long c, signed long d) {
    return i + a + b + c + d;
}

inline void __attribute__((always_inline)) f1i(va_list ap) {
    x = va_arg(ap, double); x += va_arg(ap, signed long); x += va_arg(ap, double);
}
inline void __attribute__((always_inline)) f2i(va_list ap) {
    y = va_arg(ap, int); y += va_arg(ap, signed long); y += va_arg(ap, double); f1i(ap);
}
void f2(int i, ...) { va_list ap; va_start(ap, i); f2i(ap); va_end(ap); }

int main(void) {
    char buf[40];
    OUT[0] = (p >> 8) == 0x123456L && (n >> 8) == -0x123457L && (p >> 16) == 0x1234L &&
             (n >> 16) == -0x1235L && (p >> 24) == 0x12L && (n >> 24) == -0x13L;
    OUT[1] = (p >> 31) == 0 && (n >> 31) == -1L && (pi >> 16) == 0 && (ni >> 16) == -1;
    OUT[2] = add5(zero, 0, 0, 0, 0) == 0 && add5(-1, 0, 0, 0, 0) == -1 && add5(5, 70000L, 0, 0, -1) == 70004L;
    sprintf(buf, "%d,%f,%d", 1, 1.0, 2);
    OUT[3] = strcmp(buf, "1,1.000000,2") == 0;
    f2(6, 5, 7L, 18.0, 19.0, 17L, 64.0);
    OUT[4] = x == 100 && y == 30;
    OUT[5] = 0xAA;
    return 0;
}
