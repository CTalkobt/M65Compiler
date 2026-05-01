/* printf.c — Formatted output to screen for cc45 / MEGA65
 *
 * int printf(char *fmt, ...);
 *
 * Formats and outputs each character via putchar.
 * Does NOT append a newline (unlike puts).
 *
 * Returns the number of characters written.
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
int putchar(int c);

int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char tmp[18];
    int i;
    int count = 0;

    while (*fmt) {
        if (*fmt != '%') {
            putchar(*fmt);
            count = count + 1;
            fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1;

        if (*fmt == '%') {
            putchar('%');
            count = count + 1;
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') {
            int val = va_arg(ap, int);
            itoa(val, tmp, 10);
            i = 0;
            while (tmp[i]) { putchar(tmp[i]); count = count + 1; i = i + 1; }
        } else if (*fmt == 'U' || *fmt == 'u') {
            unsigned int val = va_arg(ap, int);
            itoa(val, tmp, 10);
            i = 0;
            while (tmp[i]) {
                if (tmp[i] != '-') { putchar(tmp[i]); count = count + 1; }
                i = i + 1;
            }
        } else if (*fmt == 'X' || *fmt == 'x') {
            int val = va_arg(ap, int);
            itoa(val, tmp, 16);
            i = 0;
            while (tmp[i]) { putchar(tmp[i]); count = count + 1; i = i + 1; }
        } else if (*fmt == 'O' || *fmt == 'o') {
            int val = va_arg(ap, int);
            itoa(val, tmp, 8);
            i = 0;
            while (tmp[i]) { putchar(tmp[i]); count = count + 1; i = i + 1; }
        } else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) { putchar(*s); count = count + 1; s = s + 1; }
        } else if (*fmt == 'C' || *fmt == 'c') {
            putchar(va_arg(ap, int));
            count = count + 1;
        }
        fmt = fmt + 1;
    }

    va_end(ap);
    return count;
}
