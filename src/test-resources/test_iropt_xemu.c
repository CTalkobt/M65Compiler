/* test_iropt_xemu.c — the IR optimizer passes (cc45 IROptimizer.cpp) keep
 * program results: each item runs the patterns a pass rewrites, including
 * the cases it must leave alone. Built at the default -O2 (all passes on),
 * and by test_iropt.sh with each pass disabled in turn.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] strength reduction: * / % by powers of two, signed and unsigned
 *   [1] algebraic simplification: x+0, x*1, x*0, x|0, x^0, x&-1, x-0
 *   [2] type narrowing: byte results of int arithmetic
 *   [3] branch folding: constant conditions, break inside if, unreachable code
 *   [4] CSE: repeated expressions, reloads after a store through an alias
 *   [5] LICM: invariant and variant loop expressions, calls in the loop
 *   [6] copy propagation and address fusion: copies, array element access
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int g;
int arr[8];
unsigned char bytes[8];

static void bump(void) { g++; }
static int id(int x) { return x; }

static int strength(void) {
    volatile int vs = -7;
    volatile unsigned int vu = 1000;
    signed int s = vs;
    unsigned int u = vu;
    int ok = 1;
    ok = ok && u * 4 == 4000 && u * 8 == 8000 && u / 8 == 125 && u % 16 == 8;
    ok = ok && s * 2 == -14 && s * 4 == -28;
    ok = ok && s / 2 == -3 && s / 4 == -1 && s % 4 == -3;     /* signed: truncate toward 0 */
    ok = ok && (signed int)(u * 2u) == 2000 && (u >> 3) == 125;
    return ok;
}

static int algebraic(void) {
    volatile int vx = 1234;
    int x = vx;
    int ok = 1;
    ok = ok && x + 0 == 1234 && 0 + x == 1234 && x * 1 == 1234 && 1 * x == 1234;
    ok = ok && x * 0 == 0 && (x | 0) == 1234 && (x ^ 0) == 1234 && (x & 0xFFFF) == 1234;
    ok = ok && x - 0 == 1234 && (x << 0) == 1234 && (x - x) == 0;
    return ok;
}

static int narrowing(void) {
    volatile int va = 300, vb = 250;
    int a = va, b = vb;
    unsigned char c1 = (unsigned char)(a + b);     /* 550 & 255 = 38 */
    unsigned char c2 = (unsigned char)(a * 3);     /* 900 & 255 = 132 */
    unsigned char c3 = (unsigned char)(a - b);     /* 50 */
    int wide = a + b;                              /* the full value is still used */
    bytes[0] = (unsigned char)(a ^ b);             /* 300 ^ 250 = 470 & 255 = 214 */
    return c1 == 38 && c2 == 132 && c3 == 50 && wide == 550 && bytes[0] == 214;
}

static int branches(void) {
    volatile int v = 5;
    int r = 0;
    if (1) r += 1;
    if (0) r += 100;
    while (0) r += 1000;
    if (v > 3) r += 10; else r += 200;
    if (2 > 1) r += 20;
    {   /* break inside if: jump threading, dead block removal */
        int n = 0;
        while (n < v) {
            if (n == 3) { r += 100; break; }
            n++;
        }
        r += n;                     /* 3 */
    }
    return r == 134;
}

static int cse(void) {
    volatile int va = 7, vb = 9;
    int a = va, b = vb;
    int *p = &arr[2], *q = &arr[2];
    int t, u, x, y;
    x = a * b + 3;
    y = a * b + 3;
    arr[2] = 40;
    t = *p;
    *q = 50;                 /* p and q alias: *p must be reloaded */
    u = *p;
    g = 1;
    bump();                  /* the call changes g */
    return x == 66 && y == 66 && t == 40 && u == 50 && g == 2;
}

static int licm(void) {
    volatile int vk = 3, vn = 5;
    int k = vk, n = vn, i, s = 0, s2 = 0, s3 = 0;
    for (i = 0; i < n; i++) s += k * 7;            /* invariant: 21 per iteration */
    for (i = 0; i < n; i++) { s2 += k * 2; k++; }  /* k changes: 6+8+10+12+14 */
    g = 0;
    for (i = 0; i < n; i++) { s3 += g + 1; bump(); } /* g changes in the call: 1..5 */
    return s == 105 && s2 == 50 && s3 == 15;
}

static int copies(void) {
    volatile int v = 11;
    int a = v, b, c, d, i, s = 0;
    b = a;
    c = b;
    d = id(c);
    for (i = 0; i < 8; i++) arr[i] = i * 3 + d;    /* 11, 14, ... 32 */
    for (i = 0; i < 8; i++) s += arr[i];
    arr[arr[0] - 10] += 100;                        /* arr[1] */
    return b == 11 && c == 11 && d == 11 && s == 172 && arr[1] == 114 && arr[7] == 32;
}

int main(void) {
    OUT[0] = strength();
    OUT[1] = algebraic();
    OUT[2] = narrowing();
    OUT[3] = branches();
    OUT[4] = cse();
    OUT[5] = licm();
    OUT[6] = copies();
    OUT[7] = 0xAA;
    return 0;
}
