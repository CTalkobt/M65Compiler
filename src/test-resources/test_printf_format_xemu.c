/* test_printf_format_xemu.c -- the printf formatter (stdlib_asm/__vformat.s45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] * width (negative: left-justified), .* precision (negative: none)
 *   [1] %s with precision and padding, %c, %%, an unknown conversion
 *   [2] %n with h, (none), l; the return count
 *   [3] flags together: 0 with precision, - with 0, + and space, # and 0
 *   [4] snprintf truncation and its count
 *   [5] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[80];

int main(void) {
    int n;
    signed char nh;
    long nl;

    sprintf(buf, "[%*d][%*d][%.*d][%.*d]", 5, 42, -5, 42, 4, 7, -1, 7);
    OUT[0] = strcmp(buf, "[   42][42   ][0007][7]") == 0;

    sprintf(buf, "[%.2s][%5s][%-5s][%c][%%][%y]", "abcdef", "ab", "ab", 'Q');
    OUT[1] = strcmp(buf, "[ab][   ab][ab   ][Q][%][y]") == 0;

    n = 0; nh = 0; nl = 0;
    {
        int r = sprintf(buf, "abc%hhnde%nfgh%ln", &nh, &n, &nl);
        OUT[2] = nh == 3 && n == 5 && nl == 8L && r == 8 && strcmp(buf, "abcdefgh") == 0;
    }

    sprintf(buf, "[%06.3d][%-06d][%+d][% d][%#06x][%+05d]", 7, 7, 0, 5, 255, -3);
    OUT[3] = strcmp(buf, "[   007][7     ][+0][ 5][0x00ff][-0003]") == 0;

    n = snprintf(buf, 6, "%s-%d", "hello", 12345);
    OUT[4] = n == 11 && strcmp(buf, "hello") == 0;

    OUT[5] = 0xAA;
    return 0;
}
