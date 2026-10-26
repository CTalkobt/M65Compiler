/* stridx.c — which of several strings a string is
 *
 * stridx(s, "load", "save", "quit", NULL): 1 when s is "load", 2 for
 * "save", ... (the first that is equal), 0 when none is. cc45 calls
 * __stridx_table instead when every candidate is a string literal: the
 * candidates in one table, a count byte first, each ended by a 0 byte
 * (no NULL needed then).
 */
#include <stdarg.h>
#include <string.h>

int stridx(const char *s, ...) {
    va_list ap;
    const char *c;
    int i = 0;
    va_start(ap, s);
    while ((c = va_arg(ap, const char *)) != 0) {
        i++;
        if (strcmp(s, c) == 0) {
            va_end(ap);
            return i;
        }
    }
    va_end(ap);
    return 0;
}

int __stridx_table(const char *restrict s, const unsigned char *restrict t) {
    unsigned char n = *t++, i;
    for (i = 1; i <= n; i++) {
        const unsigned char *p = (const unsigned char *)s;
        while (*t && *p == *t) { p++; t++; }
        if (*p == *t) return i;      /* both at their 0 */
        while (*t) t++;               /* the rest of this candidate */
        t++;
    }
    return 0;
}
