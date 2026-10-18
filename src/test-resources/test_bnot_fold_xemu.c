/* test_bnot_fold_xemu.c — ~ on a constant in the constant's width (Issue #289)
 *
 * The constant folder computed ~ in 64 bits: ~0x8000 (an unsigned int) was
 * -32769, which IRBuilder made a long ("integer literal -32769 exceeds
 * 16-bit range"); ~0xFFFF was -65536.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] ~0x8000 is 0x7FFF, ~0xFFFF is 0 (an unsigned int: zero-extended to long)
 *   [1] ~0x7FFF and ~0 are signed ints: sign-extended to long
 *   [2] masks: p & ~0x8000, p & ~0xFFFF, p & ~0x00FF
 *   [3] a long constant: ~0x10000UL is 0xFFFEFFFF
 *   [4] ~ and - of a char cast are ints (~(unsigned char)1 is -2)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

NOINLINE static unsigned int mask(unsigned int p, unsigned char k) {
    if (k == 0) return p & ~0x8000;
    if (k == 1) return p & ~0xFFFF;
    return p & ~0x00FF;
}

int main(void) {
    signed long a = ~0x8000, b = ~0xFFFF, c = ~0x7FFF, d = ~0;
    unsigned long e = ~0x10000UL;
    signed int f = ~(unsigned char)1, g = -(unsigned char)1;
    OUT[0] = a == 0x7FFFL && b == 0 && sizeof(~0x8000) == 2;
    OUT[1] = c == -32768L && d == -1L;
    OUT[2] = mask(0xC3A5, 0) == 0x43A5 && mask(0xC3A5, 1) == 0 && mask(0xC3A5, 2) == 0xC300;
    OUT[3] = e == 0xFFFEFFFFUL;
    OUT[4] = f == -2 && g == -1 && ~(unsigned char)1 < 0;
    OUT[5] = 0xAA;
    return 0;
}
