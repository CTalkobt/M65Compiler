/* vsprintf.c — Formatted string output (va_list version) for cc45 / MEGA65
 *
 * int vsprintf(char *buf, char *fmt, va_list ap);
 *
 * The conversions are those of __vformat (flags, width, precision, length
 * modifiers; no %f — the float-aware sprintf adds it).
 * Returns the number of characters written (excluding NUL).
 */

#include <stdarg.h>

int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float));

/* Appends a character at *(char **)ctx */
void __vformat_buf_put(int c, void *ctx) {
    char **out = (char **)ctx;
    **out = (char)c;
    *out = *out + 1;
}

#pragma cc45 weak
int vsprintf(char *buf, char *fmt, va_list ap) {
    char *out = buf;
    int n = __vformat(__vformat_buf_put, &out, fmt, ap, 0);
    *out = 0;
    return n;
}
