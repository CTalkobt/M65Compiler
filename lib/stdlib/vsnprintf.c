/* vsnprintf.c — Size-limited formatted string output (va_list version)
 *
 * int vsnprintf(char *buf, int size, char *fmt, va_list ap);
 *
 * Writes at most size-1 characters and a terminating NUL (nothing when
 * size is 0). Returns the length the whole output would have (C99).
 */

#include <stdarg.h>

int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));

struct __vsn_ctx { char *p; int room; };

static void vsn_put(int c, void *ctx) {
    struct __vsn_ctx *k = (struct __vsn_ctx *)ctx;
    if (k->room > 1) {
        *k->p = (char)c;
        k->p = k->p + 1;
        k->room = k->room - 1;
    }
}

int vsnprintf(char *buf, int size, char *fmt, va_list ap) {
    struct __vsn_ctx k;
    k.p = buf;
    k.room = size;
    int n = __vformat(vsn_put, &k, fmt, ap, 0);
    if (size > 0) *k.p = 0;
    return n;
}
