/* sprintf.c — Formatted string output for cc45 / MEGA65
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

static void emit_itoa(char **out, int val, int base) {
    char tmp[18];
    itoa(val, tmp, base);
    int i = 0;
    while (tmp[i]) { **out = tmp[i]; *out = *out + 1; i = i + 1; }
}

static void emit_ltoa(char **out, long val, int base) {
    char tmp[34];
    ltoa(val, tmp, base);
    int i = 0;
    while (tmp[i]) { **out = tmp[i]; *out = *out + 1; i = i + 1; }
}

static void emit_utoa(char **out, unsigned int val, int base) {
    char tmp[18];
    itoa(val, tmp, base);
    int i = 0;
    while (tmp[i]) {
        if (tmp[i] != '-') { **out = tmp[i]; *out = *out + 1; }
        i = i + 1;
    }
}

static void emit_lutoa(char **out, unsigned long val, int base) {
    char tmp[34];
    ltoa(val, tmp, base);
    int i = 0;
    while (tmp[i]) {
        if (tmp[i] != '-') { **out = tmp[i]; *out = *out + 1; }
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
            fmt = fmt + 1;
            if (*fmt == 'D' || *fmt == 'd') {
                emit_ltoa(&out, va_arg(ap, long), 10);
            } else if (*fmt == 'U' || *fmt == 'u') {
                emit_lutoa(&out, va_arg(ap, long), 10);
            } else if (*fmt == 'X' || *fmt == 'x') {
                emit_ltoa(&out, va_arg(ap, long), 16);
            } else if (*fmt == 'O' || *fmt == 'o') {
                emit_ltoa(&out, va_arg(ap, long), 8);
            } else if (*fmt == 'B' || *fmt == 'b') {
                emit_ltoa(&out, va_arg(ap, long), 2);
            }
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') {
            emit_itoa(&out, va_arg(ap, int), 10);
        } else if (*fmt == 'U' || *fmt == 'u') {
            emit_utoa(&out, va_arg(ap, int), 10);
        } else if (*fmt == 'X' || *fmt == 'x') {
            emit_itoa(&out, va_arg(ap, int), 16);
        } else if (*fmt == 'O' || *fmt == 'o') {
            emit_itoa(&out, va_arg(ap, int), 8);
        } else if (*fmt == 'B' || *fmt == 'b') {
            emit_itoa(&out, va_arg(ap, int), 2);
        } else if (*fmt == 'P' || *fmt == 'p') {
            *out = '$'; out = out + 1;
            emit_itoa(&out, va_arg(ap, int), 16);
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
