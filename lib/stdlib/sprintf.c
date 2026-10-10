/* sprintf.c — Thin variadic wrapper for cc45 / MEGA65
 *
 * int sprintf(char *buf, char *fmt, ...);
 *
 * Delegates to vsprintf. The variadic wrapper receives params on
 * the stack (variadic functions always use stack convention).
 * vsprintf is non-variadic and uses SAC for its locals.
 */

#include <stdarg.h>

int vsprintf(char *buf, char *fmt, va_list ap);

#pragma cc45 weak
int sprintf(char *buf, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int count = vsprintf(buf, fmt, ap);
    va_end(ap);
    return count;
}
