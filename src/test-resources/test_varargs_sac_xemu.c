/* test_varargs_sac_xemu.c — variadic functions with static allocation
 *
 * A non-recursive variadic function keeps its named parameters and locals
 * in static storage like any other; its ... arguments stay on the stack,
 * where the entry thunk finds them (<f>__va, read by va_start).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int arguments; none at all
 *   [1] long arguments after a char and a long named parameter
 *   [2] mixed int/long by a format string
 *   [3] a variadic function calling another with its own ... (the two
 *       argument lists must not mix), and through va_list (vsum)
 *   [4] sprintf (the library's, variadic)
 *   [5] $AA sentinel
 */
#include <stdarg.h>
#include <stdio.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

N static int sum(int n, ...) { va_list ap; int s = 0; va_start(ap, n); while (n--) s += va_arg(ap, int); va_end(ap); return s; }
N static long lsum(char k, long a, ...) { va_list ap; long s = a; va_start(ap, a); while (k--) s += va_arg(ap, long); va_end(ap); return s; }
N static int mixed(const char *fmt, ...) {
    va_list ap; int r = 0;
    va_start(ap, fmt);
    for (; *fmt; fmt++) { if (*fmt == 'i') r += va_arg(ap, int); else if (*fmt == 'l') r += (int)va_arg(ap, long); }
    va_end(ap);
    return r;
}
N static int vsum(int n, va_list ap) { int s = 0; while (n--) s += va_arg(ap, int); return s; }
N static int outer(int n, ...) {
    va_list ap; int a, b;
    va_start(ap, n);
    a = va_arg(ap, int);
    b = sum(2, a * 10, 7);              /* another variadic call in between */
    a += vsum(n - 1, ap);               /* the rest through va_list */
    va_end(ap);
    return a + b;
}
static char buf[40];

int main(void) {
    OUT[0] = sum(3, 10, 20, 30) == 60 && sum(0) == 0;
    OUT[1] = lsum(2, 100000L, 200000L, 300000L) == 600000L;
    OUT[2] = mixed("ili", 1, 70000L, 5) == 1 + 4464 + 5;
    /* outer(3, 4, 5, 6): a = 4, b = 40 + 7, a += 5 + 6 */
    OUT[3] = outer(3, 4, 5, 6) == 4 + 47 + 11;
    sprintf(buf, "%d-%s-%ld", 42, "ab", 123456L);
    OUT[4] = buf[0] == '4' && buf[3] == 'a' && buf[6] == '1' && buf[11] == '6' && buf[12] == 0;
    OUT[5] = 0xAA;
    return 0;
}
