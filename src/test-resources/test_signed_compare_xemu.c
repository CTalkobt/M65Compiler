/* test_signed_compare_xemu.c — signed 16/32-bit comparisons across signs
 *
 * Expected at $C0C0: 01 00 01 00 01 01 01 00 00 AA
 */
#pragma cc45 no_zp_save
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
volatile signed long a = -100000L, b = 7L, c = -5L, d = -100000L;
volatile signed int x = -12, y = 5;
void main() {
    R(0) = (a < b); R(1) = (b < a); R(2) = (a < c); R(3) = (c < a); R(4) = (a <= d); R(5) = (b > c);
    R(6) = (x < y); R(7) = (y < x); R(8) = (x >= y);
    R(9) = 0xAA;
}
