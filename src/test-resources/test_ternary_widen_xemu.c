/* test_ternary_widen_xemu.c — a ?: between a long and a narrower value
 * (Issue #310): the narrower arm is widened to the result (its top bytes
 * were left as they were: c ? x : 0 returned garbage in the top 16 bits)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] c ? x : 0 and c ? 0 : x, the registers dirty before
 *   [1] c ? g : 0 with a global long
 *   [2] an int and a char arm against a long (zero-extended)
 *   [3] a signed int arm against a signed long (sign-extended: -1)
 *   [4] the ?: kept in a variable, then used
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned long g = 0x12345678UL;

NOINLINE static unsigned long pick(unsigned char c, unsigned long x) { return c ? x : 0; }
NOINLINE static unsigned long pick2(unsigned char c, unsigned long x) { return c ? 0 : x; }
NOINLINE static unsigned long pickg(unsigned char c) { return c ? g : 0; }
NOINLINE static unsigned long pickint(unsigned char c, unsigned long x, unsigned int i) { return c ? x : i; }
NOINLINE static unsigned long pickchar(unsigned char c, unsigned long x, unsigned char b) { return c ? b : x; }
NOINLINE static signed long picksigned(unsigned char c, signed long x, signed int i) { return c ? x : i; }
NOINLINE static unsigned long kept(unsigned char c, unsigned long x) {
    unsigned long r = x ^ 0xFFFF0000UL;
    r = c ? r : 0;
    return r + 1;
}
NOINLINE static void dirty(void) { volatile unsigned long t = 0xFFFFFFFFUL; t++; }

int main(void) {
    dirty();
    OUT[0] = pick(0, 0xAABBCCDDUL) == 0 && pick(1, 0xAABBCCDDUL) == 0xAABBCCDDUL && pick2(1, 0xAABBCCDDUL) == 0;
    dirty();
    OUT[1] = pickg(0) == 0 && pickg(1) == 0x12345678UL;
    dirty();
    OUT[2] = pickint(0, 0xAABBCCDDUL, 0xFFFF) == 0xFFFFUL && pickchar(1, 0xAABBCCDDUL, 0xFE) == 0xFEUL;
    dirty();
    OUT[3] = picksigned(0, 5, -1) == -1L && picksigned(1, -70000L, -1) == -70000L;
    dirty();
    OUT[4] = kept(0, 0x11223344UL) == 1 && kept(1, 0x11223344UL) == 0xEEDD3345UL;
    OUT[5] = 0xAA;
    return 0;
}
