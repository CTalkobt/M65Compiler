/* vsnprintf.c — Size-limited formatted string output (va_list version)
 *
 * int vsnprintf(char *buf, int size, char *fmt, va_list ap);
 */

#include <stdarg.h>

int vsprintf(char *buf, char *fmt, va_list ap);

int vsnprintf(char *buf, int size, char *fmt, va_list ap) {
    if (size <= 0) return 0;

    char tmp[256];
    int len = vsprintf(tmp, fmt, ap);

    int copy = len;
    if (copy >= size) copy = size - 1;

    int i;
    for (i = 0; i < copy; i++) {
        buf[i] = tmp[i];
    }
    buf[i] = 0;

    return len;
}
