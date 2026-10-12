/* vprintf.c — Formatted output to screen (va_list version) for cc45 / MEGA65
 *
 * int vprintf(char *fmt, va_list ap);
 *
 * The conversions are those of __vformat (no %f — the float-aware printf
 * adds it). Returns the number of characters written.
 */

#include <stdarg.h>

__fastcall__ int putchar(int c);
int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));

/* Prints a character (the context is unused) */
void __vformat_con_put(int c, void *ctx) {
    putchar(c);
}

#pragma cc45 weak
int vprintf(char *fmt, va_list ap) {
    return __vformat(__vformat_con_put, 0, fmt, ap, 0);
}
