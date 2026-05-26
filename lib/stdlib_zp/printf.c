/* printf.c — Formatted output to screen for cc45 / MEGA65 (ZP calling convention)
 *
 * int printf(char *fmt, ...);
 *
 * Supports: %d %u %x %o %s %c %% and long variants %ld %lu %lx %lo.
 *
 * Returns the number of characters written.
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);
int putchar(int c);

static int emit_buf(char *buf, int skip_minus) {
    int i = 0; int c = 0;
    while (buf[i]) {
        if (!skip_minus || buf[i] != '-') { putchar(buf[i]); c = c + 1; }
        i = i + 1;
    }
    return c;
}

int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

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
            char tmp[34];
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') {
                ltoa(va_arg(ap, long), tmp, 10);
                count = count + emit_buf(tmp, 0);
            } else if (*fmt == 'U' || *fmt == 'u') {
                ltoa(va_arg(ap, long), tmp, 10);
                count = count + emit_buf(tmp, 1);
            } else if (*fmt == 'X' || *fmt == 'x') {
                ltoa(va_arg(ap, long), tmp, 16);
                count = count + emit_buf(tmp, 0);
            } else if (*fmt == 'O' || *fmt == 'o') {
                ltoa(va_arg(ap, long), tmp, 8);
                count = count + emit_buf(tmp, 0);
            }
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 10);
            count = count + emit_buf(tmp, 0);
        } else if (*fmt == 'U' || *fmt == 'u') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 10);
            count = count + emit_buf(tmp, 1);
        } else if (*fmt == 'X' || *fmt == 'x') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 16);
            count = count + emit_buf(tmp, 0);
        } else if (*fmt == 'O' || *fmt == 'o') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 8);
            count = count + emit_buf(tmp, 0);
        } else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) { putchar(*s); count = count + 1; s = s + 1; }
        } else if (*fmt == 'C' || *fmt == 'c') {
            putchar(va_arg(ap, int)); count = count + 1;
        }
        fmt = fmt + 1;
    }

    va_end(ap);
    return count;
}
