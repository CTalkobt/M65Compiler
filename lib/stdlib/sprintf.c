/* sprintf.c — Formatted string output for cc45 / MEGA65
 *
 * int sprintf(char *buf, char *fmt, ...);
 *
 * Supported: %d %u %x %o %b %s %c %p %% %ld %lu %lx %lo %lb
 *
 * Note: Format specifiers are matched in PETSCII encoding (case-swapped
 * relative to ASCII) because string literals go through .text conversion.
 * This is the integer-only version (weak). The float-aware version in
 * printf_float.c provides a strong override when float support is needed.
 *
 * Returns the number of characters written (excluding NUL).
 */

#include <stdarg.h>

int itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

static void vemit_buf(char **out, char *buf, int skip_minus) {
    int i = 0;
    while (buf[i]) {
        if (!skip_minus || buf[i] != '-') { **out = buf[i]; *out = *out + 1; }
        i = i + 1;
    }
}

static void emit_int(char **out, int val, int base, int skip_minus) {
    char tmp[18];
    itoa(val, tmp, base);
    vemit_buf(out, tmp, skip_minus);
}

static void emit_long(char **out, long val, int base, int skip_minus) {
    char tmp[34];
    ltoa(val, tmp, base);
    vemit_buf(out, tmp, skip_minus);
}

int vsprintf(char *buf, char *fmt, va_list ap);

#pragma cc45 weak
int sprintf(char *buf, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = vsprintf(buf, fmt, ap);
    va_end(ap);
    return count;
}

#pragma cc45 weak
int vsprintf(char *buf, char *fmt, va_list ap) {
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
            if (*fmt == 'D' || *fmt == 'd') emit_long(&out, va_arg(ap, long), 10, 0);
            else if (*fmt == 'U' || *fmt == 'u') emit_long(&out, va_arg(ap, long), 10, 1);
            else if (*fmt == 'X' || *fmt == 'x') emit_long(&out, va_arg(ap, long), 16, 0);
            else if (*fmt == 'O' || *fmt == 'o') emit_long(&out, va_arg(ap, long), 8, 0);
            else if (*fmt == 'B' || *fmt == 'b') emit_long(&out, va_arg(ap, long), 2, 0);
            fmt = fmt + 1;
            continue;
        }

        if (*fmt == 'D' || *fmt == 'd') emit_int(&out, va_arg(ap, int), 10, 0);
        else if (*fmt == 'U' || *fmt == 'u') emit_int(&out, va_arg(ap, int), 10, 1);
        else if (*fmt == 'X' || *fmt == 'x') emit_int(&out, va_arg(ap, int), 16, 0);
        else if (*fmt == 'O' || *fmt == 'o') emit_int(&out, va_arg(ap, int), 8, 0);
        else if (*fmt == 'B' || *fmt == 'b') emit_int(&out, va_arg(ap, int), 2, 0);
        else if (*fmt == 'P' || *fmt == 'p') {
            *out = '$'; out = out + 1;
            emit_int(&out, va_arg(ap, int), 16, 0);
        } else if (*fmt == 'S' || *fmt == 's') {
            char *s = (char *)va_arg(ap, int);
            while (*s) { *out = *s; out = out + 1; s = s + 1; }
        } else if (*fmt == 'C' || *fmt == 'c') {
            *out = (char)va_arg(ap, int); out = out + 1;
        }
        fmt = fmt + 1;
    }

    *out = 0;
    return (int)(out - buf);
}
