/* sscanf.c — Formatted string input for cc45 / MEGA65
 *
 * sscanf / vsscanf on the scanf engine (scanf_core.c), reading a string.
 */

#include "scanf_core.h"

int vsscanf(const char *str, const char *fmt, va_list ap) {
    return __vscanf_core(NULL, str, fmt, ap);
}

int sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vsscanf(str, fmt, ap);
    va_end(ap);
    return r;
}
