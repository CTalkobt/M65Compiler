/* test_varargs_char_xemu.c — char arguments through "..." are promoted
 * to int: zero-extended (unsigned char) or sign-extended (signed char)
 *
 * (Only A was loaded for a char argument: va_arg(ap, int) read the stale
 * high byte left in X, e.g. $12C8 for $C8 after an int $1234.)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned char after an int: high byte 0
 *   [1] signed char -5: $FFFB; _Bool
 *   [2] sprintf %d / %u / %c / %x with char arguments
 *   [3] $AA sentinel
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int got[4];

__attribute__((noinline)) static void take(int n, ...) {
    va_list ap;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) got[i] = va_arg(ap, unsigned int);
    va_end(ap);
}

int main(void) {
    volatile unsigned char uc = 0xC8;
    volatile signed char sc = -5;
    volatile unsigned int big = 0x1234;
    volatile _Bool b = 1;
    char buf[32];

    take(3, big, uc, (unsigned char)(uc + 1));
    OUT[0] = got[0] == 0x1234 && got[1] == 0x00C8 && got[2] == 0x00C9;
    take(3, big, sc, b);
    OUT[1] = got[1] == 0xFFFB && got[2] == 1;
    sprintf(buf, "%d %u %c %x", sc, uc, (char)0x41, uc);
    /* ($41 is "a" in a cc45 string literal: both are PETSCII) */
    OUT[2] = strcmp(buf, "-5 200 a c8") == 0;
    OUT[3] = 0xAA;
    return 0;
}
