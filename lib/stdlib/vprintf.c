/* vprintf.c — Formatted output to screen (va_list version) for cc45 / MEGA65
 *
 * int vprintf(char *fmt, va_list ap);
 *
 * Supports: %d %u %x %o %s %c %% and long variants %ld %lu %lx %lo.
 *
 * Returns the number of characters written.
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);
int putchar(int c);

static int vemit_buf(char *buf, int skip_minus) {
    int i = 0; int c = 0;
    while (buf[i]) {
        if (!skip_minus || buf[i] != '-') { putchar(buf[i]); c = c + 1; }
        i = i + 1;
    }
    return c;
}

static int emit_int(int val, int base, int skip_minus) {
    char tmp[18];
    itoa(val, tmp, base);
    return vemit_buf(tmp, skip_minus);
}

static int emit_long(long val, int base, int skip_minus) {
    char tmp[34];
    ltoa(val, tmp, base);
    return vemit_buf(tmp, skip_minus);
}

#pragma cc45 weak
int vprintf(char *fmt, va_list ap) {
    int count = 0;

    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt); count = count + 1; fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1;

        if (*fmt == '%') {
            putchar('%'); count = count + 1; fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'L' || *fmt == 'l') {
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') count = count + emit_long(va_arg(ap, long), 10, 0);
            else if (*fmt == 'U' || *fmt == 'u') count = count + emit_long(va_arg(ap, long), 10, 1);
            else if (*fmt == 'X' || *fmt == 'x') count = count + emit_long(va_arg(ap, long), 16, 0);
            else if (*fmt == 'O' || *fmt == 'o') count = count + emit_long(va_arg(ap, long), 8, 0);
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') count = count + emit_int(va_arg(ap, int), 10, 0);
        else if (*fmt == 'U' || *fmt == 'u') count = count + emit_int(va_arg(ap, int), 10, 1);
        else if (*fmt == 'X' || *fmt == 'x') count = count + emit_int(va_arg(ap, int), 16, 0);
        else if (*fmt == 'O' || *fmt == 'o') count = count + emit_int(va_arg(ap, int), 8, 0);
        else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) { putchar(*s); count = count + 1; s = s + 1; }
        } else if (*fmt == 'C' || *fmt == 'c') {
            putchar(va_arg(ap, int)); count = count + 1;
        }
        fmt = fmt + 1;
    }

    return count;
}
