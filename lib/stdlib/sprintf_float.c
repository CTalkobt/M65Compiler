/* sprintf_float.c — Float-aware sprintf for cc45 / MEGA65
 *
 * Provides strong _sprintf that overrides the weak integer-only version.
 * Supports all integer format specifiers plus %f for float/double.
 */

#include <stdarg.h>

__fastcall__ int __pf_format(char *buf, float val, int conv, int prec, int alt);   /* stdlib_asm/pf_format.s45 */

/* Referenced by cc45 when sprintf is called with a float argument, so this
 * member (and its strong sprintf) is linked instead of the integer-only one */
void sprintf_float(void) {}

/* __vformat (stdlib_asm/__vformat.s45) and its callbacks use the ZP convention */
__fastcall__ int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));
__fastcall__ void __vformat_buf_put(int c, void *ctx);

int sprintf(char *buf, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *out = buf;
    int count = __vformat(__vformat_buf_put, &out, fmt, ap, __pf_format);
    *out = 0;
    va_end(ap);
    return count;
}
