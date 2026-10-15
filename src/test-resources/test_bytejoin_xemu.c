/* test_bytejoin_xemu.c — values put together from bytes (code generation)
 *
 * a | b << 8 | (long)c << 16: the OR loads each byte of the result from
 * where its source lives, without the zero-extensions, shifts and ORs in
 * between. Loads through constant addresses are absolute (lda $D770).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] bytes read through constant addresses (the math unit's inputs
 *       $D770-$D773 read back what was written), 16 and 32 bits
 *   [1] parameters and locals joined, 16 and 32 bits, a constant byte
 *   [2] masks and shifts both ways (byte swaps: a leaf used twice)
 *   [3] truncation (the high word of a long) in a join
 *   [4] sources whose bytes overlap are not joined: still OR-ed
 *   [5] in a loop, the sources changing each time round
 *   [6] a sign-extended byte (all its bytes count)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define R(a) (*(volatile unsigned char *)(a))
#define NOINLINE __attribute__((noinline))

NOINLINE static unsigned rd16(void) {
    return R(0xD770) | ((unsigned)R(0xD771) << 8);
}

NOINLINE static unsigned long rd32(void) {
    return R(0xD770) | ((unsigned)R(0xD771) << 8) | ((unsigned long)R(0xD772) << 16) |
           ((unsigned long)R(0xD773) << 24);
}

NOINLINE static unsigned j2(unsigned char lo, unsigned char hi) {
    return lo | ((unsigned)hi << 8);
}

NOINLINE static unsigned long j4(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    return ((unsigned long)d << 24) | ((unsigned long)c << 16) | ((unsigned)b << 8) | a;
}

NOINLINE static unsigned long j3k(unsigned char a, unsigned b) {
    unsigned char x = a;
    return 0x7F000000UL | ((unsigned long)b << 8) | x;
}

NOINLINE static unsigned swap16(unsigned x) {
    return (x >> 8) | (x << 8);
}

NOINLINE static unsigned long swapw(unsigned long l) {
    return (l >> 16) | (l << 16);
}

NOINLINE static unsigned keep_hi(unsigned x, unsigned char lo) {
    return (x & 0xFF00) | lo;
}

NOINLINE static unsigned trunc_join(unsigned long l) {
    return (unsigned)(l >> 24) | ((unsigned)(l & 0xFF) << 8);
}

NOINLINE static unsigned overlap(unsigned a, unsigned char b) {
    return a | ((unsigned)b << 4);
}

NOINLINE static unsigned overlap2(unsigned a, unsigned char b) {
    return a | ((unsigned)b << 8);
}

NOINLINE static unsigned loop_join(const unsigned char *p, unsigned char n) {
    unsigned char i;
    unsigned s = 0;
    for (i = 0; i < n; i++) {
        unsigned char lo = p[2 * i], hi = p[2 * i + 1];
        s ^= lo | ((unsigned)hi << 8);
    }
    return s;
}

NOINLINE static unsigned sext_join(unsigned char a, unsigned char b) {
    return (unsigned)(signed char)a | ((unsigned)b << 8);
}

static const unsigned char words[] = {0x34, 0x12, 0x78, 0x56, 0xBC, 0x9A};

int main(void) {
    R(0xD770) = 0x11; R(0xD771) = 0x22; R(0xD772) = 0x33; R(0xD773) = 0x44;
    OUT[0] = rd16() == 0x2211 && rd32() == 0x44332211UL;
    OUT[1] = j2(0x34, 0x12) == 0x1234 && j4(1, 2, 3, 4) == 0x04030201UL &&
             j4(0xFF, 0, 0x80, 0) == 0x008000FFUL && j3k(0x56, 0x1234) == 0x7F123456UL;
    OUT[2] = swap16(0x1234) == 0x3412 && swapw(0x12345678UL) == 0x56781234UL &&
             keep_hi(0xABCD, 0x12) == 0xAB12;
    OUT[3] = trunc_join(0x12345678UL) == 0x7812;
    OUT[4] = overlap(0x1001, 0xFF) == 0x1FF1 && overlap2(0x00F0, 0x0F) == 0x0FF0;
    OUT[5] = loop_join(words, 3) == (0x1234 ^ 0x5678 ^ 0x9ABC);
    OUT[6] = sext_join(0x80, 0x12) == 0xFF80 && sext_join(0x7F, 0x12) == 0x127F;
    OUT[7] = 0xAA;
    return 0;
}
