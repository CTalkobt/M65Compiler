/* test_incdec_xemu.c — ++/-- on variables updated in place (inc/dec/inw/dew,
 * the ca45 inc.16f/dec.16f of a stack-frame variable)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] --n / n-- on an int across the byte boundary (256 -> 255 -> ... 0)
 *       in a loop condition, static allocation
 *   [1] the same on a char; ++c wrapping 255 -> 0
 *   [2] pre/post values used in expressions (x = --n + n, y = n++ * 2)
 *   [3] the same in a function with a stack frame (#pragma cc45 recurse):
 *       a counter past 255 and back
 *   [4] a leaf function (variables in ZP registers) and a long (not in place)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

static unsigned char calls;
NOINLINE static void tick(void) { calls++; }

NOINLINE static unsigned pre_int(unsigned n) { unsigned s = 0; while (--n > 0) { s++; tick(); } return s; }
NOINLINE static unsigned post_int(unsigned n) { unsigned s = 0; while (n--) { s++; tick(); } return s + n; }
NOINLINE static unsigned pre_char(unsigned char n) { unsigned s = 0; while (--n) { s++; tick(); } return s; }
NOINLINE static unsigned post_char(unsigned char n) { unsigned s = 0; while (n--) { s++; tick(); } return s + n; }
NOINLINE static unsigned char wrap(unsigned char c) { tick(); ++c; tick(); return c; }

NOINLINE static unsigned exprs(unsigned n) {
    unsigned x, y;
    tick();
    x = --n + n;          /* n = n0-1: x = 2(n0-1) (unsequenced in C; both reads see n0-1 here) */
    tick();
    y = n++ * 2;          /* y = 2(n0-1), n = n0 */
    return x + y + n;
}

#pragma cc45 recurse
NOINLINE static unsigned framed(unsigned n) {
    unsigned i, s = 0, d = n;
    for (i = 0; i < n; i++) s++;          /* i++ in place (inc.16f) */
    while (d--) s++;                      /* d-- in place (dec.16f) */
    return s;
}

NOINLINE static unsigned leaf(unsigned n) { unsigned s = 0; unsigned char c = 3; while (--n) s += c++; return s; }

NOINLINE static unsigned long lng(unsigned long n) { unsigned long s = 0; while (n--) { s++; tick(); } return s; }

int main(void) {
    unsigned char c0;
    calls = 0;
    OUT[0] = pre_int(300) == 299 && post_int(300) == 299 /* 300 + 0xFFFF in 16 bits */ && pre_int(1) == 0;
    c0 = calls;
    OUT[1] = pre_char(200) == 199 && post_char(5) == 5 + 255 && wrap(255) == 0 && wrap(7) == 8;
    OUT[2] = exprs(10) == 18 + 18 + 10;
    OUT[3] = framed(300) == 600 && framed(256) == 512 && framed(0) == 0;
    /* leaf: n = 5 -> 4 iterations: c = 3, 4, 5, 6 */
    OUT[4] = leaf(5) == 18 && lng(260) == 260 && c0 == (unsigned char)(299 + 300);
    OUT[5] = 0xAA;
    return 0;
}
