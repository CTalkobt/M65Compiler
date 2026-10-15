/* conio_printf.c — formatted console output (<conio.h>)
 *
 * The rest of conio — the screen, the cursor, putch / cputs, getch — is in
 * assembly: lib/stdlib_asm/conio.s45.
 */

#pragma cc45 signed_default

#include <conio.h>
#include <stdio.h>
#include <stdarg.h>

int cprintf(const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, (char *)format, ap);
    va_end(ap);

    cputs(buffer);

    return count;
}

int cprintfxy(int x, int y, const char *format, ...) {
    va_list ap;
    char buffer[256];
    int count;

    va_start(ap, format);
    count = vsprintf(buffer, (char *)format, ap);
    va_end(ap);

    cputsxy(x, y, buffer);

    return count;
}
