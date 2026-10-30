/* conio_printf.c — the variadic part of <conio.h>: cprintf, cprintfxy,
 * cscanf
 *
 * They hand their arguments to vcprintf / vcscanf; the rest of conio is in
 * assembly: lib/stdlib_asm/conio.s45.
 */

#pragma cc45 signed_default

#include <conio.h>
#include <stdarg.h>

int cprintf(const char *format, ...) {
    va_list ap;
    int count;

    va_start(ap, format);
    count = vcprintf(format, ap);
    va_end(ap);
    return count;
}

int cprintfxy(int x, int y, const char *format, ...) {
    va_list ap;
    int count;

    gotoxy(x, y);
    va_start(ap, format);
    count = vcprintf(format, ap);
    va_end(ap);
    return count;
}

int cscanf(const char *format, ...) {
    va_list ap;
    int count;

    va_start(ap, format);
    count = vcscanf(format, ap);
    va_end(ap);
    return count;
}
