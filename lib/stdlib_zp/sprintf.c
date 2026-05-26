/* sprintf.c — Formatted string output for cc45 / MEGA65 (ZP calling convention)
 *
 * int sprintf(char *buf, char *fmt, ...);
 *
 * Supported: %d %u %x %o %b %s %c %p %% %ld %lu %lx %lo %lb
 *
 * Note: Format specifiers are matched in PETSCII encoding (case-swapped
 * relative to ASCII) because string literals go through .text conversion.
 *
 * Returns the number of characters written (excluding NUL).
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

static void emit_buf(char **out, char *buf, int skip_minus) {
    int i = 0;
    while (buf[i]) {
        if (!skip_minus || buf[i] != '-') { **out = buf[i]; *out = *out + 1; }
        i = i + 1;
    }
}

int sprintf(char *buf, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char *out = buf;

    while (*fmt) {
        if (*fmt != '%') {
            *out = *fmt; out = out + 1; fmt = fmt + 1;
            continue;
        }
        fmt = fmt + 1;

        if (*fmt == '%') {
            *out = '%'; out = out + 1; fmt = fmt + 1;
            continue;
        }

        /* Check for 'l' length modifier followed by specifier */
        if (*fmt == 'L' || *fmt == 'l') {
            char tmp[34];
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') {
                ltoa(va_arg(ap, long), tmp, 10);
                emit_buf(&out, tmp, 0);
            } else if (*fmt == 'U' || *fmt == 'u') {
                ltoa(va_arg(ap, long), tmp, 10);
                emit_buf(&out, tmp, 1);
            } else if (*fmt == 'X' || *fmt == 'x') {
                ltoa(va_arg(ap, long), tmp, 16);
                emit_buf(&out, tmp, 0);
            } else if (*fmt == 'O' || *fmt == 'o') {
                ltoa(va_arg(ap, long), tmp, 8);
                emit_buf(&out, tmp, 0);
            } else if (*fmt == 'B' || *fmt == 'b') {
                ltoa(va_arg(ap, long), tmp, 2);
                emit_buf(&out, tmp, 0);
            }
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 10);
            emit_buf(&out, tmp, 0);
        } else if (*fmt == 'U' || *fmt == 'u') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 10);
            emit_buf(&out, tmp, 1);
        } else if (*fmt == 'X' || *fmt == 'x') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 16);
            emit_buf(&out, tmp, 0);
        } else if (*fmt == 'O' || *fmt == 'o') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 8);
            emit_buf(&out, tmp, 0);
        } else if (*fmt == 'B' || *fmt == 'b') {
            char tmp[18];
            itoa(va_arg(ap, int), tmp, 2);
            emit_buf(&out, tmp, 0);
        } else if (*fmt == 'P' || *fmt == 'p') {
            char tmp[18];
            *out = '$'; out = out + 1;
            itoa(va_arg(ap, int), tmp, 16);
            emit_buf(&out, tmp, 0);
        } else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) { *out = *s; out = out + 1; s = s + 1; }
        } else if (*fmt == 'C' || *fmt == 'c') {
            *out = (char)va_arg(ap, int); out = out + 1;
        }
        fmt = fmt + 1;
    }

    *out = 0;
    va_end(ap);
    return (int)(out - buf);
}
