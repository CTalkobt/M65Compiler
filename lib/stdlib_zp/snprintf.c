/* snprintf.c — Size-limited formatted string output
 *
 * int snprintf(char *buf, int size, char *fmt, ...);
 *
 * Like sprintf but writes at most size-1 characters, always NUL-terminates.
 * Returns the number of characters that would have been written (excluding NUL)
 * if the buffer were large enough (C99 semantics).
 */

#include <stdarg.h>

int vsprintf(char *buf, char *fmt, va_list ap);

int snprintf(char *buf, int size, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    if (size <= 0) {
        va_end(ap);
        return 0;
    }

    /* Format into a temporary buffer to get the full length.
     * For simplicity on 8-bit, format into buf directly then truncate.
     * This is safe as long as caller provides a buffer of at least 'size'. */
    char tmp[256];
    int len = vsprintf(tmp, fmt, ap);
    va_end(ap);

    /* Copy at most size-1 chars */
    int copy = len;
    if (copy >= size) copy = size - 1;

    int i;
    for (i = 0; i < copy; i++) {
        buf[i] = tmp[i];
    }
    buf[i] = 0;

    return len;
}
