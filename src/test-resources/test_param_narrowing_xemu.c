/* test_param_narrowing_xemu.c — int parameters every call passes 0-255
 *
 * A static function's int parameter that every call passes a byte value
 * (a constant, an unsigned char, x & 0xFF) is passed as a byte when each of
 * its uses costs nothing zero-extended: the function reads it in 16-bit
 * additions, subtractions, and/xor and compares from the byte where it
 * lives, with 0 for the high byte (cc45 IRCodeGen, not a 16-bit temporary).
 * Char variables zero-extended in such operations are read the same way.
 * Run in both conventions, with and without -fsigned-default.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] add / sub with the parameter on either side (carries and borrows)
 *   [1] and / xor
 *   [2] unsigned and equality compares, the parameter on either side
 *   [3] signed < / >= compares, the parameter on either side
 *   [4] char variables zero-extended in place
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

volatile unsigned int w1 = 0x12F0, w2 = 0x0080, wbig = 0x0100, wmax = 0xFFFF;
volatile signed int sneg = -3, spos = 300, ssmall = 5;
volatile unsigned char cv = 0xF5, cz = 0, cmask = 0x0F;

N static unsigned int add_l(unsigned int p, unsigned int x) { return p + x; }
N static unsigned int add_r(unsigned int x, unsigned int p) { return x + p; }
N static unsigned int sub_l(unsigned int p, unsigned int x) { return p - x; }
N static unsigned int sub_r(unsigned int x, unsigned int p) { return x - p; }
N static unsigned int and_p(unsigned int p, unsigned int x) { return p & x; }
N static unsigned int xor_p(unsigned int p, unsigned int x) { return x ^ p; }

// x against p: == 1, != 2, < 4, <= 8, > 16, >= 32
N static unsigned char cmp_r(unsigned int p, unsigned int x) {
    return (x == p) | (x != p) << 1 | (x < p) << 2 | (x <= p) << 3 | (x > p) << 4 | (x >= p) << 5;
}
// p against x
N static unsigned char cmp_l(unsigned int p, unsigned int x) {
    return (p == x) | (p != x) << 1 | (p < x) << 2 | (p <= x) << 3 | (p > x) << 4 | (p >= x) << 5;
}
// x < p 1, x >= p 2, p < x 4, p >= x 8
N static unsigned char cmp_s(signed int p, signed int x) {
    return (x < p) | (x >= p) << 1 | (p < x) << 2 | (p >= x) << 3;
}

N static unsigned char locals(void) {
    unsigned char c = cv;
    unsigned int x = w1;
    signed int s = sneg;
    unsigned char ok = 1;
    if ((unsigned int)(x + c) != 0x13E5) ok = 0;
    if ((unsigned int)(x - c) != 0x11FB) ok = 0;
    if ((unsigned int)(c - x) != 0xEE05) ok = 0;
    if ((x & c) != 0xF0) ok = 0;
    if ((x ^ c) != 0x1205) ok = 0;
    if (!(x > c)) ok = 0;
    if (c >= x) ok = 0;
    if (s >= (signed int)c) ok = 0;
    if (!((signed int)c >= s)) ok = 0;
    return ok;
}

int main(void) {
    unsigned char ok = 1;

    if (add_l(200, w1) != 0x13B8) ok = 0;
    if (add_l(cv, w1) != 0x13E5) ok = 0;
    if (add_r(w1, 255) != 0x13EF) ok = 0;
    if (add_r(wmax, 1) != 0) ok = 0;
    if (sub_l(5, w2) != 0xFF85) ok = 0;
    if (sub_l(200, w2) != 0x48) ok = 0;
    if (sub_r(w2, 0x81) != 0xFFFF) ok = 0;
    if (sub_r(w1, w1 & 0xFF) != 0x1200) ok = 0;
    OUT[0] = ok;

    ok = 1;
    if (and_p(cv, w1) != 0xF0) ok = 0;
    if (and_p(cmask, wmax) != 0x0F) ok = 0;
    if (xor_p(cv, w1) != 0x1205) ok = 0;
    if (xor_p(0, w1) != 0x12F0) ok = 0;
    OUT[1] = ok;

    ok = 1;
    if (cmp_r(0x80, w2) != 41) ok = 0;
    if (cmp_r(0x80, wbig) != 50) ok = 0;
    if (cmp_r(0xFF, w2) != 14) ok = 0;
    if (cmp_r(cz, wmax) != 50) ok = 0;
    if (cmp_l(0x80, wbig) != 14) ok = 0;
    if (cmp_l(0x80, w2) != 41) ok = 0;
    if (cmp_l(cv, w2) != 50) ok = 0;
    if (cmp_l(cz, 0) != 41) ok = 0;
    OUT[2] = ok;

    ok = 1;
    if (cmp_s(5, sneg) != 9) ok = 0;
    if (cmp_s(5, spos) != 6) ok = 0;
    if (cmp_s(5, ssmall) != 10) ok = 0;
    if (cmp_s(200, sneg) != 9) ok = 0;
    OUT[3] = ok;

    OUT[4] = locals();
    OUT[5] = 0xAA;
    return 0;
}
