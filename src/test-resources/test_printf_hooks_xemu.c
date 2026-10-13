/* test_printf_hooks_xemu.c -- the printf formatter's digit hooks
 *
 * __vformat links the digits of integer conversions only when a format
 * string needs them: cc45 sees the literal formats (%d -> __fmt_utoa,
 * %ld -> __fmt_ultoa) and, for a format that is not a literal, links
 * everything. test_xemu.sh also checks that a %s-only program links no hook.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] %d %i %u with flags, width, precision
 *   [1] %x %X %o %b %p and # flags
 *   [2] %ld %lu %lx %lld (32-bit digits)
 *   [3] a wrapper passing a variable format to vsprintf
 *   [4] %s and %c (no hook)
 *   [5] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[64];

static void fmt(char *f, ...) {
    va_list ap;
    va_start(ap, f);
    vsprintf(buf, f, ap);
    va_end(ap);
}

int main(void) {
    char *two = "%d/%lu";

    sprintf(buf, "%d|%+i|% d|%5u|%-4d|%05d|%.3d|%.0d", -42, 7, 9, 300, 12, -7, 5, 0);
    OUT[0] = strcmp(buf, "-42|+7| 9|  300|12  |-0007|005|") == 0;

    sprintf(buf, "%x|%X|%o|%b|%p|%#x|%#o|%#b", 0xbeef, 0xbeef, 8, 5, 0x2a, 255, 8, 2);
    OUT[1] = strcmp(buf, "beef|BEEF|10|101|$002a|0xff|010|0b10") == 0;

    sprintf(buf, "%ld|%lu|%lx|%lld", -100000L, 3000000000UL, 0xdeadbeefUL, 123456789LL);
    OUT[2] = strcmp(buf, "-100000|3000000000|deadbeef|123456789") == 0;

    fmt(two, 1234, 4000000000UL);
    OUT[3] = strcmp(buf, "1234/4000000000") == 0;

    sprintf(buf, "[%s:%c]", "ok", '!');
    OUT[4] = strcmp(buf, "[ok:!]") == 0;

    OUT[5] = 0xAA;
    return 0;
}
