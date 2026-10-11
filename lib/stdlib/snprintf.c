/* snprintf.c — Size-limited formatted string output
 *
 * int snprintf(char *buf, int size, char *fmt, ...);
 *
 * Like sprintf but writes at most size-1 characters, always NUL-terminates
 * (when size > 0). Returns the number of characters that would have been
 * written (excluding NUL) if the buffer were large enough (C99 semantics).
 */

#include <stdarg.h>

int vsnprintf(char *buf, int size, char *fmt, va_list ap);

int snprintf(char *buf, int size, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return len;
}
