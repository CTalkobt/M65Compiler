/* test_inline_xemu.c — automatic inlining (-O2) keeps results.
 * Each callee is small enough to be inlined at -O2; test_xemu.sh builds it
 * at -O2 and at -O0 (no inlining), in both conventions.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the inlined return value has the callee's type (long from an int function)
 *   [1] a parameter the callee assigns (n = n - 1 in a loop) is a real variable
 *   [2] a caller array named like the callee's pointer parameter
 *   [3] a caller local named like a global the callee uses
 *   [4] a callee that takes a parameter's address (not inlined: called)
 *   [5] repeated inlining into one function (two calls of each callee)
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int g = 7;
char buf[8];

static unsigned long big(void) { return 32768; }
/* always_inline: larger than the default inline size limit */
static inline __attribute__((always_inline)) int rep(char *p, int c, int n) { int k = 0; while (n > 0) { p[k++] = (char)c; n = n - 1; } return k; }
static int same(int *C, int *Cref) { return C == Cref; }
static int useg(void) { return g + 1; }
static int addr(int x) { int *p = &x; *p += 5; return x; }

static int t0(void) {
    unsigned long v = big();
    if (v & 32768) v |= 65536;
    return v == 98304UL;
}

static int t1(void) {
    int n = 3, k = rep(buf, 'x', n);
    return k == 3 && n == 3 && buf[0] == 'x' && buf[2] == 'x' && buf[3] == 0;
}

static int t2(void) {
    int C[2];
    return same(C, C) && !same(C, C + 1);
}

static int t3(void) {
    int g = 100;            /* hides the global in this function only */
    return useg() == 8 && g == 100;
}

static int t4(void) {
    return addr(1) == 6 && addr(10) == 15;
}

static int t5(void) {
    int a = rep(buf + 4, 'y', 2), b = rep(buf + 6, 'z', 1);
    return a == 2 && b == 1 && buf[4] == 'y' && buf[5] == 'y' && buf[6] == 'z' && useg() + useg() == 16;
}

int main(void) {
    OUT[0] = t0();
    OUT[1] = t1();
    OUT[2] = t2();
    OUT[3] = t3();
    OUT[4] = t4();
    OUT[5] = t5();
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
