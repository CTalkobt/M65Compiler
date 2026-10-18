/* test_purity_infer_xemu.c — purity inference (-fipa-pure-const): functions
 * found const / pure are optimized like declared ones, and functions with
 * effects (memory reads, writes, loops, recursion, unknown calls) are not
 *
 * Results at $C0C0 (01 = pass):
 *   [0] inferred const functions (sq, clamp, tot: calls sq) give their values,
 *       also repeated in one expression
 *   [1] an inferred const call in a loop condition (hoisted): right count
 *   [2] an inferred pure function (only calls strlen) after its argument's
 *       bytes change: the new length (a store between, not reused)
 *   [3] a helper reading the raster register through a volatile pointer,
 *       polled until the line changes: not pure (hoisted, it never would)
 *   [4] a function writing a global, its result unused: still called
 *   [5] a loop (pop) and recursion (fact): right values
 *   [6] a function calling an unknown one: still called
 *   [7] $AA sentinel
 */
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define RASTER ((volatile unsigned char *)0xD012)
#define NOINLINE __attribute__((noinline))

unsigned char g;
unsigned int count;
static char buf[8] = "abc";

NOINLINE static int sq(int x) { return x * x; }
NOINLINE static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
NOINLINE static int tot(int a) { return sq(a) + sq(a + 1); }
NOINLINE static unsigned int len2(const char *s) { return strlen(s) + 1; }
NOINLINE static unsigned char peek(volatile unsigned char *p) { return *p; }
NOINLINE static int bump(void) { g++; return g; }
NOINLINE static int pop(unsigned char x) { int n = 0; while (x) { n += x & 1; x >>= 1; } return n; }
NOINLINE static int fact(int n) { return n < 2 ? 1 : n * fact(n - 1); }
NOINLINE void other(void) { count++; }
NOINLINE static int ext(int a) { other(); return a; }

NOINLINE static int loop(int n) {
    int t = 0, i;
    for (i = 0; i < sq(n); i++) t += i;
    return t;
}

int main(void) {
    unsigned char start, ok;
    unsigned int n1;

    OUT[0] = sq(7) + sq(7) == 98 && clamp(12, 0, 9) == 9 && clamp(3, 5, 9) == 5 && tot(3) == 25;
    OUT[1] = loop(3) == 36 && loop(0) == 0;

    n1 = len2(buf);
    buf[3] = 'd'; buf[4] = 0;
    OUT[2] = n1 == 4 && len2(buf) == 5;

    start = peek(RASTER);
    while (peek(RASTER) == start) ;
    OUT[3] = 1;

    g = 0;
    bump();
    bump();
    OUT[4] = g == 2;

    OUT[5] = pop(0xB5) == 5 && fact(5) == 120;

    count = 0;
    ext(1);
    ok = ext(2) == 2 && count == 2;
    OUT[6] = ok;
    OUT[7] = 0xAA;
    return 0;
}
