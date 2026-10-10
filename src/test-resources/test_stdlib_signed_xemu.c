/* test_stdlib_signed_xemu.c — library results that are negative: atoi, min, max, div
 *
 * Expected at $C0C0: 85 FF F4 FF 05 00 FD FF FF FF AA
 */
#pragma cc45 no_zp_save
#include <stdlib.h>
#include <math.h>
#define W(i,v) do { signed int _t = (v); *(volatile unsigned char *)(0xC0C0 + 2*(i)) = (unsigned char)_t; *(volatile unsigned char *)(0xC0C1 + 2*(i)) = (unsigned char)(_t >> 8); } while (0)
volatile signed int neg = -12;
void main() {
    W(0, atoi("-123"));
    W(1, min(neg, 5));
    W(2, max(neg, 5));
    div_t d = div(-7, 2);
    W(3, d.quot);
    W(4, d.rem);
    *(volatile unsigned char *)0xC0CA = 0xAA;
}
