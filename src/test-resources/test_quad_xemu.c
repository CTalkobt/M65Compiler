/* test_quad_xemu.c — 32-bit values through the 45GS02's Q instructions:
 * ca45 merges a byte-at-a-time 32-bit store (sta/stx/sty/stz) into stq and
 * a load into ldq (not when the flags of the last load are used), and the
 * 32-bit shift simops are aslq / lsrq / rolq / rorq / asrq, cmp.32 against
 * memory cpq
 *
 * Results at $C0C0 (01 = pass):
 *   [0] long globals copied, added, read back (ldq / stq absolute)
 *   [1] a long's truth value and its sign, high byte zero or not
 *   [2] shifts by constants: << 1, 3, 9; >> 1, 7, 17 (unsigned and signed)
 *   [3] shifts by a variable count, both ways, a negative value >>
 *   [4] unsigned compares of longs (<, <=, ==, >) in memory and registers
 *   [5] char and int zero-extended to long, mixed in sums
 *   [6] long multiply, divide and remainder through the math unit (Q moves
 *       in and out), globals and locals, a product over 16 bits
 *   [7] a long plus a byte (the carry into X, Y, Z only as far as it goes):
 *       carries out of each byte, through all four, a global += 1
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned long ga = 0x12345678UL, gb = 0x0F0F0F0FUL, gc;
signed long gs = -0x10001L;
volatile unsigned char k3 = 3, k17 = 17;

NOINLINE static unsigned char truth(unsigned long v) { return v ? 1 : 0; }
NOINLINE static unsigned char negative(signed long v) { return v < 0; }
NOINLINE static unsigned char below(unsigned long a, unsigned long b) { return a < b; }
NOINLINE static unsigned char atmost(unsigned long a, unsigned long b) { return a <= b; }
NOINLINE static unsigned char above(unsigned long a, unsigned long b) { return a > b; }
NOINLINE static unsigned long mul(unsigned long a, unsigned long b) { return a * b; }
NOINLINE static unsigned long quo(unsigned long a, unsigned long b) { return a / b; }
NOINLINE static unsigned long rem(unsigned long a, unsigned long b) { return a % b; }
NOINLINE static unsigned long add5(unsigned long v) { return v + 5; }
NOINLINE static unsigned long add255(unsigned long v) { return v + 255; }
NOINLINE static unsigned long widen(unsigned char c, unsigned int i) { return (unsigned long)c + i + 0x10000UL; }

int main(void) {
    unsigned char ok;
    unsigned long v;
    signed long s;

    gc = ga + gb;
    OUT[0] = gc == 0x21436587UL && ga == 0x12345678UL;

    ok = truth(0) == 0 && truth(0x01000000UL) == 1 && truth(1) == 1 && truth(0x80000000UL) == 1;
    ok = ok && negative(-1L) && !negative(0x7FFFFFFFL) && negative(gs) && !negative(0);
    OUT[1] = ok;

    v = ga;
    ok = (v << 1) == 0x2468ACF0UL && (v << 3) == 0x91A2B3C0UL && (v << 9) == 0x68ACF000UL;
    ok = ok && (v >> 1) == 0x091A2B3CUL && (v >> 7) == 0x002468ACUL && (v >> 17) == 0x091AUL;
    s = gs;
    ok = ok && (s >> 1) == -0x8001L && (s >> 17) == -1L && (s << 4) == -0x100010L;
    OUT[2] = ok;

    ok = (v << k3) == 0x91A2B3C0UL && (v >> k17) == 0x091AUL && (s >> k3) == -0x2001L;
    OUT[3] = ok;

    ok = below(1, 2) && !below(2, 1) && below(0x00FFFFFFUL, 0x01000000UL) && !below(0x80000000UL, 0x7FFFFFFFUL);
    ok = ok && atmost(5, 5) && atmost(0x10000UL, 0x10001UL) && !atmost(0x10002UL, 0x10001UL);
    ok = ok && above(0xFFFFFFFFUL, 0) && !above(gb, ga) && (ga == 0x12345678UL) && !(gb == ga);
    OUT[4] = ok;

    OUT[5] = widen(200, 60000u) == 0x10000UL + 60200UL && widen(0, 0) == 0x10000UL;
    ok = mul(0x12345UL, 0x10UL) == 0x123450UL && mul(65535UL, 65537UL) == 0xFFFFFFFFUL && ga * 2 == 0x2468ACF0UL;
    ok = ok && quo(0x87654321UL, 0x10000UL) == 0x8765UL && rem(0x87654321UL, 0x10000UL) == 0x4321UL;
    ok = ok && quo(ga, gb) == 1 && rem(ga, gb) == ga - gb && ga / 7 == 0x299C335UL;
    OUT[6] = ok;
    ok = add5(1) == 6 && add5(0xFB) == 0x100 && add5(0xFFFB) == 0x10000UL && add5(0xFFFFFB) == 0x1000000UL;
    ok = ok && add5(0xFFFFFFFBUL) == 0 && add255(0x1FF01UL) == 0x20000UL && add255(0x1234FFUL) == 0x1235FEUL;
    gc = 0x00FFFFFFUL;
    gc += 1;
    OUT[7] = ok && gc == 0x01000000UL;
    OUT[8] = 0xAA;
    return 0;
}
