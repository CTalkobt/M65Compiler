/* test_zp_args_xemu.c — the table-driven parameter thunk of ZP-convention
 * SAC functions (__zp_args, Issue #292): the parameters reach their homes
 * (static storage, the base-page block) and survive calls
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int, char, int, long parameters, all used after a call
 *   [1] a parameter used only before the first call (kept in $10-$2F, its
 *       slot skipped in the list) between copied ones
 *   [2] a float parameter and a pointer parameter after a call
 *   [3] chars only (short thunks stay inline) and many small parameters
 *   [4] a recursive caller of thunked functions (values not mixed up)
 *   [5] the functions called through a pointer (always the thunk entry)
 *   [6] a struct-returning function's parameters (the hidden pointer first)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

static volatile unsigned char sink;

NOINLINE static void clobber(void) { sink++; }

NOINLINE static void wipe(unsigned int a, unsigned int b, unsigned int c, unsigned int d,
                          unsigned int e, unsigned int f, unsigned int g, unsigned int h) {
    sink = (unsigned char)(a + b + c + d + e + f + g + h);
}

NOINLINE static unsigned long four(unsigned int a, unsigned char b, unsigned int c, unsigned long d) {
    wipe(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);
    return a + b + c + d;
}

NOINLINE static unsigned int skipped(unsigned int a, unsigned int early, unsigned char b, unsigned int c) {
    unsigned int e = early * 3;          /* early: used before the call only */
    wipe(1, 2, 3, 4, 5, 6, 7, 8);
    return a + b + c + e;
}

NOINLINE static unsigned char fp(float f, unsigned char *p, unsigned int n) {
    wipe(9, 9, 9, 9, 9, 9, 9, 9);
    p[0] = (unsigned char)(f * 2.0f);
    p[1] = (unsigned char)n;
    return p[0] + p[1];
}

NOINLINE static unsigned char chars(unsigned char a, unsigned char b) {
    clobber();
    return a - b;
}

NOINLINE static unsigned int many(unsigned char a, unsigned char b, unsigned char c, unsigned char d,
                                  unsigned char e, unsigned char f, unsigned char g) {
    wipe(0, 0, 0, 0, 0, 0, 0, 0);
    return a + 2 * b + 3 * c + 4 * d + 5 * e + 6 * f + 7 * g;
}

#pragma cc45 recurse
NOINLINE static unsigned long rec(unsigned char n, unsigned int k) {
    if (n == 0) return 0;
    return four(k, n, k + 1, 100000UL) + rec(n - 1, k + 10);
}

struct pair { unsigned int x, y; };

NOINLINE static struct pair mk(unsigned int a, unsigned int b) {
    struct pair p;
    wipe(7, 7, 7, 7, 7, 7, 7, 7);
    p.x = a + 1;
    p.y = b + 2;
    return p;
}

int main(void) {
    unsigned char buf[2];
    unsigned long (*pf)(unsigned int, unsigned char, unsigned int, unsigned long) = four;
    unsigned int (*ps)(unsigned int, unsigned int, unsigned char, unsigned int) = skipped;
    struct pair q;
    unsigned long r;

    OUT[0] = four(1000, 200, 3000, 70000UL) == 74200UL && four(1, 2, 3, 4) == 10;
    OUT[1] = skipped(100, 7, 20, 3000) == 3141 && skipped(0, 0, 255, 0) == 255;
    OUT[2] = fp(10.5f, buf, 300) == 21 + 44 && buf[0] == 21 && buf[1] == 44;
    OUT[3] = chars(200, 50) == 150 && many(1, 2, 3, 4, 5, 6, 7) == 140;
    /* rec(3, 5): four(5,3,6,..) + four(15,2,16,..) + four(25,1,26,..) */
    r = rec(3, 5);
    OUT[4] = r == 3 * 100000UL + (5 + 3 + 6) + (15 + 2 + 16) + (25 + 1 + 26);
    OUT[5] = pf(10, 20, 30, 40) == 100 && ps(1, 2, 3, 4) == 14;
    q = mk(40, 50);
    OUT[6] = q.x == 41 && q.y == 52;
    OUT[7] = 0xAA;
    return 0;
}
