/* printf.c — Formatted output to screen for cc45 / MEGA65
 *
 * int printf(char *fmt, ...);
 *
 * Supports: %d %u %x %o %s %c %% and long variants %ld %lu %lx %lo.
 * This is the integer-only version (weak). The float-aware version in
 * printf_float.c provides a strong override when float support is needed.
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

int vprintf(char *fmt, va_list ap);

#pragma cc45 weak
int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = vprintf(fmt, ap);
    va_end(ap);
    return count;
}
