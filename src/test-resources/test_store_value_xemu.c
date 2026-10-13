/* test_store_value_xemu.c — the value of a store through a pointer or to a
 * global, taken where it lives
 *
 * A byte cut from a variable ((char)i, *p = c with int c) is stored from the
 * variable's low byte; a constant used only as stored values is not put in a
 * register first.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] *p = c with int c, in a loop with a call (static allocation)
 *   [1] (char) of int and long variables through a pointer and to a global
 *   [2] constants (char, int, long) through pointers
 *   [3] constants to globals, one constant used twice
 *   [4] the same in a function with a stack frame (#pragma cc45 recurse)
 *   [5] a constant both stored and used in arithmetic
 *   [6] a call's result stored into a variable and used again (read from
 *       the variable): char, int and pointer results, the value of the
 *       assignment in an expression; a narrowing store (int into char);
 *       the variable changed after the store; *(p = e) reads one byte
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static const unsigned char src[] = {1, 2, 3, 13, 4, 5, 6, 0};
static unsigned char pos;
static int next(void) { return src[pos] ? src[pos++] : -1; }

__attribute__((noinline)) static char *readline(char *buf, int n) {
    char *p = buf;
    int c;
    while (--n > 0 && (c = next()) != -1 && c != 13) *p++ = c;
    *p = 0;
    return buf;
}

static unsigned char gc;
static unsigned gi;
static unsigned long gl;

__attribute__((noinline)) static void cut(unsigned char *p, unsigned i, unsigned long l) {
    p[0] = (unsigned char)i;
    p[1] = (unsigned char)l;
    gc = (unsigned char)i;
}

__attribute__((noinline)) static void consts(unsigned char *cp, unsigned *ip, unsigned long *lp) {
    *cp = 0x5A;
    *ip = 0x1234;
    *lp = 0x89ABCDEFUL;
    cp[1] = 0;
}

__attribute__((noinline)) static void gconsts(void) {
    gc = 7;
    gi = 7;
    gl = 0x01020304UL;
}

#pragma cc45 recurse
__attribute__((noinline)) static void framed(unsigned char *p, unsigned i, unsigned *ip) {
    p[0] = (unsigned char)i;
    p[1] = 0x33;
    *ip = 0xBEEF;
}

__attribute__((noinline)) static unsigned both(unsigned *ip, unsigned x) {
    unsigned k = 300;
    *ip = k;
    return x + k;
}

__attribute__((noinline)) static int ret_i(int v) { return v * 3; }
__attribute__((noinline)) static unsigned char ret_c(unsigned char v) { return (unsigned char)(v + 200); }
__attribute__((noinline)) static unsigned char *ret_p(unsigned char *p) { return p + 1; }

__attribute__((noinline)) static unsigned char results(void) {
    int i; unsigned char c, nc; unsigned char *p; int sum;
    static unsigned char arr[3] = {5, 6, 7};
    sum = (i = ret_i(100)) + i;               /* 600 */
    if ((c = ret_c(100)) != 44 || c + 1 != 45) return 0;
    if (*(p = ret_p(arr)) != 6 || p[1] != 7) return 0;
    if (*(p = arr + 2) != 7) return 0;      /* *(p = e): the pointee's width */
    nc = (unsigned char)ret_i(100);            /* 300 -> 44: narrowing */
    if (nc != 44) return 0;
    c = ret_c(1);                              /* 201 */
    c++;
    if (c != 202) return 0;
    return sum == 600;
}

int main(void) {
    char line[8];
    unsigned char b[4];
    unsigned w[2];
    unsigned long l;

    readline(line, sizeof line);
    OUT[0] = line[0] == 1 && line[2] == 3 && line[3] == 0 && pos == 4;

    b[0] = b[1] = 0xEE;
    cut(b, 0x1241, 0x77665544UL);
    OUT[1] = b[0] == 0x41 && b[1] == 0x44 && gc == 0x41;

    b[1] = 0xEE;
    consts(b, w, &l);
    OUT[2] = b[0] == 0x5A && b[1] == 0 && w[0] == 0x1234 && l == 0x89ABCDEFUL;

    gconsts();
    OUT[3] = gc == 7 && gi == 7 && gl == 0x01020304UL;

    framed(b, 0x2299, w + 1);
    OUT[4] = b[0] == 0x99 && b[1] == 0x33 && w[1] == 0xBEEF;

    OUT[5] = both(w, 5) == 305 && w[0] == 300;
    OUT[6] = results();
    OUT[7] = 0xAA;
    return 0;
}
