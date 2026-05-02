/* sprintf.c — Formatted string output for cc45 / MEGA65
 *
 * int sprintf(char *buf, char *fmt, ...);
 *
 * Supported format specifiers:
 *   %d   — signed decimal integer (16-bit)
 *   %u   — unsigned decimal integer (16-bit)
 *   %x   — unsigned hexadecimal (16-bit)
 *   %s   — NUL-terminated string
 *   %c   — single character
 *   %%   — literal '%'
 *   %o   — unsigned octal (16-bit)
 *   %b   — unsigned binary (16-bit)
 *   %p   — pointer (hex with $)
 *   %ld  — signed decimal long (32-bit)
 *   %lu  — unsigned decimal long (32-bit)
 *   %lx  — unsigned hexadecimal long (32-bit)
 *   %lo  — unsigned octal long (32-bit)
 *
 * Note: Format specifiers are matched in PETSCII encoding (case-swapped
 * relative to ASCII) because string literals go through .text conversion.
 *
 * Returns the number of characters written (excluding NUL).
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

int sprintf(char *buf, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *out = buf;
    char tmp[34];
    int i;

    while (*fmt) {
        if (*fmt != '%') {
            *out = *fmt;
            out = out + 1;
            fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1;

        if (*fmt == '%') {
            *out = '%';
            out = out + 1;
            fmt = fmt + 1;
            continue;
        }

        /* Check for 'l' length modifier (long) */
        int isLong = 0;
        if (*fmt == 'L' || *fmt == 'l') {
            isLong = 1;
            fmt = fmt + 1;
        }

        /* PETSCII: .text swaps case, so 'd' ($64) becomes 'D' ($44).
         * Char literals are ASCII. Compare against uppercase for
         * lowercase format specifiers in the source. */
        if (*fmt == 'D' || *fmt == 'd') {
            if (isLong) {
                long val = va_arg(ap, long);
                ltoa(val, tmp, 10);
            } else {
                int val = va_arg(ap, int);
                itoa(val, tmp, 10);
            }
            i = 0;
            while (tmp[i]) {
                *out = tmp[i];
                out = out + 1;
                i = i + 1;
            }
        } else if (*fmt == 'U' || *fmt == 'u') {
            if (isLong) {
                unsigned long val = va_arg(ap, long);
                ltoa(val, tmp, 10);
            } else {
                unsigned int val = va_arg(ap, int);
                itoa(val, tmp, 10);
            }
            i = 0;
            while (tmp[i]) {
                if (tmp[i] != '-') {
                    *out = tmp[i];
                    out = out + 1;
                }
                i = i + 1;
            }
        } else if (*fmt == 'X' || *fmt == 'x') {
            if (isLong) {
                long val = va_arg(ap, long);
                ltoa(val, tmp, 16);
            } else {
                int val = va_arg(ap, int);
                itoa(val, tmp, 16);
            }
            i = 0;
            while (tmp[i]) {
                *out = tmp[i];
                out = out + 1;
                i = i + 1;
            }
        } else if (*fmt == 'O' || *fmt == 'o') {
            if (isLong) {
                long val = va_arg(ap, long);
                ltoa(val, tmp, 8);
            } else {
                int val = va_arg(ap, int);
                itoa(val, tmp, 8);
            }
            i = 0;
            while (tmp[i]) {
                *out = tmp[i];
                out = out + 1;
                i = i + 1;
            }
        } else if (*fmt == 'B' || *fmt == 'b') {
            if (isLong) {
                long val = va_arg(ap, long);
                ltoa(val, tmp, 2);
            } else {
                int val = va_arg(ap, int);
                itoa(val, tmp, 2);
            }
            i = 0;
            while (tmp[i]) {
                *out = tmp[i];
                out = out + 1;
                i = i + 1;
            }
        } else if (*fmt == 'P' || *fmt == 'p') {
            int val = va_arg(ap, int);
            *out = '$';
            out = out + 1;
            itoa(val, tmp, 16);
            i = 0;
            while (tmp[i]) {
                *out = tmp[i];
                out = out + 1;
                i = i + 1;
            }
        } else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) {
                *out = *s;
                out = out + 1;
                s = s + 1;
            }
        } else if (*fmt == 'C' || *fmt == 'c') {
            char c = va_arg(ap, int);
            *out = c;
            out = out + 1;
        }
        fmt = fmt + 1;
    }

    *out = 0;
    va_end(ap);
    return (int)(out - buf);
}
