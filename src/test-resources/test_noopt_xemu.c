/* test_noopt_xemu.c — #pragma cc45 optimize(off) / optimize(on)
 *
 * The functions between the two pragmas (and one with the pragma in its
 * body) are compiled as written: no IR pass, no inlining in or of them, and
 * ca45's passes leave their code alone. They must still compute what their
 * optimized twins do.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] arithmetic a pass would rewrite (x * 4, x / 2, x % 8, x + 0)
 *   [1] loops (a sum, a nested count, a while with && )
 *   [2] dead stores and copies kept, the last value stored wins
 *   [3] calls: a small static function that would be inlined, recursion
 *   [4] the pragma inside a function body: that function only
 *   [5] functions after optimize(on) are optimized again, and agree
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char g;
static unsigned char buf[40];

static unsigned char twice(unsigned char x) { return x + x; }

/* the optimized twins */
static unsigned arith_o(unsigned x) { return x * 4 + x / 2 + x % 8 + 0; }
static unsigned sum_o(unsigned char n) { unsigned s = 0; unsigned char i; for (i = 0; i < n; i++) s += i; return s; }
static unsigned nest_o(void) { unsigned c = 0; unsigned char i, j; for (i = 0; i < 5; i++) for (j = 0; j < i; j++) c++; return c; }
static unsigned char len_o(const unsigned char *s, unsigned char max) { unsigned char n = 0; while (n < max && s[n]) n++; return n; }

#pragma cc45 optimize(off)
static unsigned arith(unsigned x) { return x * 4 + x / 2 + x % 8 + 0; }
static unsigned sum(unsigned char n) { unsigned s = 0; unsigned char i; for (i = 0; i < n; i++) s += i; return s; }
static unsigned nest(void) { unsigned c = 0; unsigned char i, j; for (i = 0; i < 5; i++) for (j = 0; j < i; j++) c++; return c; }
static unsigned char len(const unsigned char *s, unsigned char max) { unsigned char n = 0; while (n < max && s[n]) n++; return n; }
static unsigned char stores(unsigned char a) {
    unsigned char t = a;
    t = t;
    g = 1;
    g = 2;
    g = a;
    return t;
}
static unsigned char calls(unsigned char a) { return twice(a) + twice(twice(a)); }
#pragma cc45 recurse
static unsigned fact(unsigned char n) { return n <= 1 ? 1 : n * fact(n - 1); }
#pragma cc45 optimize(on)

static unsigned inbody(unsigned char n) {
#pragma cc45 optimize(off)
    unsigned s = 0;
    unsigned char i;
    for (i = 0; i < n; i++) s += twice(i);
    return s;
}

static unsigned after(unsigned char n) { unsigned s = 0; unsigned char i; for (i = 0; i < n; i++) s += twice(i); return s; }

int main(void) {
    unsigned char k;
    for (k = 0; k < 10; k++) buf[k] = (unsigned char)(k + 1);
    buf[10] = 0;

    OUT[0] = arith(100) == arith_o(100) && arith(7) == 28 + 3 + 7 && arith(1234) == arith_o(1234);
    OUT[1] = sum(10) == 45 && sum(10) == sum_o(10) && nest() == nest_o() && nest() == 10 &&
             len(buf, 40) == 10 && len(buf, 4) == 4 && len_o(buf, 40) == 10;
    OUT[2] = stores(9) == 9 && g == 9;
    OUT[3] = calls(3) == 6 + 12 && fact(6) == 720;
    OUT[4] = inbody(5) == 20;
    OUT[5] = after(5) == 20 && after(5) == inbody(5);
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
