/* conio_fcm_printf.c — fcm_cprintf (<conio.h>; apart from conio_fcm.c for
 * vsprintf) */
#include <conio.h>
#include <stdio.h>
#include <stdarg.h>

int fcm_cprintf(int x, int y, int fg, int bg, const char *format, ...) {
    static char buffer[256];
    va_list args;
    int count;

    va_start(args, format);
    count = vsnprintf(buffer, sizeof buffer, (char *)format, args);
    va_end(args);
    if (count >= 0) fcm_cputs(x, y, buffer, fg, bg);
    return count;
}
