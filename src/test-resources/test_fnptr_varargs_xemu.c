/* test_fnptr_varargs_xemu.c -- calls through pointers to variadic functions
 * (int (*)(char *, ...)): the stack convention, char arguments promoted
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sprintf through a pointer: char, int, long arguments
 *   [1] a variadic function of the program through a typedef'd pointer
 *   [2] a variadic pointer as a struct member
 *   [3] a pointer declared (...) alone, and one as a parameter
 *   [4] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[40];

static int sum(int n, ...) {
    va_list ap;
    int s = 0;
    va_start(ap, n);
    while (n-- > 0) s += va_arg(ap, int);
    va_end(ap);
    return s;
}

typedef int (*summer)(int, ...);
struct ops { int (*fmt)(char *, char *, ...); summer add; };

static int apply(int (*f)(int, ...), int a, int b) { return f(2, a, b); }

int main(void) {
    int (*sp)(char *, char *, ...) = sprintf;
    summer s = sum;
    struct ops o;
    char c = 'Z';
    o.fmt = sprintf;
    o.add = sum;

    sp(buf, "%c%d/%ld", c, -5, 100000L);
    OUT[0] = strcmp(buf, "Z-5/100000") == 0;
    OUT[1] = s(3, 10, 20, (char)7) == 37;
    o.fmt(buf, "[%s]", "m");
    OUT[2] = strcmp(buf, "[m]") == 0 && o.add(1, 41) == 41;
    OUT[3] = apply(sum, 30, 12) == 42;
    OUT[4] = 0xAA;
    return 0;
}
