/* printf_float.c — Float-aware printf for cc45 / MEGA65
 *
 * Provides a strong _printf that overrides the weak integer-only version.
 * cc45 references _printf_float (the sentinel below) when a variadic call
 * passes a float, which links this member and its printf.
 *
 * The formatting of %f %e %g (%F %E %G) is __pf_format
 * (stdlib_asm/pf_format.s45): the digits come from the BASIC 65 ROM's number
 * formatting (FOUT), rounded and laid out as C's conversions.
 */

#include <stdarg.h>

/* Referenced by cc45 (see above) */
void printf_float(void) {}

/* the float formatter (stdlib_asm/pf_format.s45), ZP convention */
__fastcall__ int __pf_format(char *buf, float val, int conv, int prec, int alt);

/* --- printf with %f support (strong, overrides weak integer-only version) --- */

/* __vformat (stdlib_asm/__vformat.s45) and its callbacks use the ZP convention */
__fastcall__ int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));
__fastcall__ void __vformat_con_put(int c, void *ctx);

int printf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = __vformat(__vformat_con_put, 0, fmt, ap, __pf_format);
    va_end(ap);
    return count;
}
