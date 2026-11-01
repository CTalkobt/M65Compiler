#pragma cc45 no_zp_save
/* test_signed_mixed_xemu.c — a signed operand with a narrower unsigned one
 * (#377): the common type is the signed one, so a product of a signed long
 * and an unsigned char divided, shifted or compared stays signed; folded
 * constants the same
 *
 * Expected at $C0C0: F0 FF 70 FE FF FC FF F0 FF 01 01 3D FF AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
signed int sc = -26; unsigned char vi = 30; unsigned int u = 200; signed char c = -16;
void put16(unsigned char at, signed int v) { R(at) = (unsigned char)v; R(at + 1) = (unsigned char)((unsigned int)v >> 8); }
void main() {
    signed long l;
    put16(0, (signed int)((signed long)sc * vi / 48));          /* -16 */
    l = ((signed long)c * u) >> 3;                              /* -400 */
    put16(2, (signed int)l);
    R(4) = (unsigned char)(l >> 24);
    put16(5, (signed int)-80 / (unsigned char)20);              /* -4, folded */
    put16(7, (signed int)((signed long)-26 * (unsigned char)30 / 48));  /* -16, folded */
    R(9) = ((signed long)sc * vi) < 0;                          /* 1 */
    R(10) = ((signed long)sc * vi % 7 == -3);                   /* -780 % 7 = -3: 1 */
    put16(11, (signed int)sc * vi / 4);                         /* -195 */
    R(13) = 0xAA;
}
