/* printf.c — Formatted output to screen for cc45 / MEGA65
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

static int emit_itoa_pc(int val, int base) {
    char tmp[18];
    itoa(val, tmp, base);
    int i = 0; int c = 0;
    while (tmp[i]) { putchar(tmp[i]); c = c + 1; i = i + 1; }
    return c;
}

static int emit_ltoa_pc(long val, int base) {
    char tmp[34];
    ltoa(val, tmp, base);
    int i = 0; int c = 0;
    while (tmp[i]) { putchar(tmp[i]); c = c + 1; i = i + 1; }
    return c;
}

static int emit_utoa_pc(unsigned int val, int base) {
    char tmp[18];
    itoa(val, tmp, base);
    int i = 0; int c = 0;
    while (tmp[i]) {
        if (tmp[i] != '-') { putchar(tmp[i]); c = c + 1; }
        i = i + 1;
    }
    return c;
}

static int emit_lutoa_pc(unsigned long val, int base) {
    char tmp[34];
    ltoa(val, tmp, base);
    int i = 0; int c = 0;
    while (tmp[i]) {
        if (tmp[i] != '-') { putchar(tmp[i]); c = c + 1; }
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
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') {
                count = count + emit_ltoa_pc(va_arg(ap, long), 10);
            } else if (*fmt == 'U' || *fmt == 'u') {
                count = count + emit_lutoa_pc(va_arg(ap, long), 10);
            } else if (*fmt == 'X' || *fmt == 'x') {
                count = count + emit_ltoa_pc(va_arg(ap, long), 16);
            } else if (*fmt == 'O' || *fmt == 'o') {
                count = count + emit_ltoa_pc(va_arg(ap, long), 8);
            }
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') {
            count = count + emit_itoa_pc(va_arg(ap, int), 10);
        } else if (*fmt == 'U' || *fmt == 'u') {
            count = count + emit_utoa_pc(va_arg(ap, int), 10);
        } else if (*fmt == 'X' || *fmt == 'x') {
            count = count + emit_itoa_pc(va_arg(ap, int), 16);
        } else if (*fmt == 'O' || *fmt == 'o') {
            count = count + emit_itoa_pc(va_arg(ap, int), 8);
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
