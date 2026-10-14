/* test_zp_params_xemu.c — a leaf function's parameters used in the ZP
 * parameter block
 *
 * A ZP-convention function that makes no calls (and has no inline asm or
 * float code) uses its arguments where the caller put them ($10...), with no
 * copy into storage at the entry.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] char, int, long and pointer parameters, several in one call
 *   [1] parameters changed in the body (n--, p++, x += ...)
 *   [2] a parameter whose address is taken (stays in storage)
 *   [3] a vector passed by value (its elements addressed: stays in storage)
 *   [4] called through function pointers; a leaf calling nothing but used
 *       from a non-leaf caller that has its own parameters live
 *   [5] the same in a function with a stack frame (#pragma cc45 recurse)
 *   [6] a function that makes calls: parameters used only before its first
 *       call stay in the block too (a, b); one also used after a call (c),
 *       one passed to the first call itself (d), one used in a loop (n)
 *       are copied
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

typedef unsigned char v4qi __attribute__((vector_size(4)));

NOINLINE static unsigned long mix(unsigned char c, unsigned i, unsigned long l, const unsigned char *p) {
    return l + i + c + p[1];
}

NOINLINE static unsigned sum(const unsigned char *p, unsigned char n) {
    unsigned s = 0;
    while (n--) s += *p++;
    return s;
}

NOINLINE static unsigned bump(unsigned x, unsigned char k) {
    x += k;
    k <<= 1;
    return x + k;
}

NOINLINE static unsigned char via_addr(unsigned char v) {
    unsigned char *q = &v;
    *q += 1;
    return v;
}

NOINLINE static unsigned char vsum(v4qi v) { return v[0] + v[1] + v[2] + v[3]; }

#pragma cc45 recurse
NOINLINE static unsigned framed(const unsigned char *p, unsigned char n) {
    unsigned s = 0;
    while (n--) s += *p++;
    return s;
}

static unsigned char a[5] = {1, 2, 3, 4, 250};

NOINLINE static unsigned twice(unsigned x) { return x * 2; }

/* a, b: read before the first call only; c after a call; d an argument of
   the first call; n in the loop (another block) */
NOINLINE static unsigned nonleaf(unsigned a, unsigned b, unsigned c, unsigned d, unsigned char n) {
    unsigned s = a + b;                 /* a, b here only */
    s += twice(d);                      /* d: the first call's argument */
    s += c;                             /* c after a call */
    while (n--) s += twice(1);
    return s;
}

NOINLINE static unsigned outer(const unsigned char *p, unsigned char n, unsigned k) {
    /* p, n and k live across the calls: this function is not a leaf */
    return sum(p, n) + sum(p + 1, n - 1) + k;
}

int main(void) {
    unsigned (*fs)(const unsigned char *, unsigned char) = sum;
    unsigned (*fb)(unsigned, unsigned char) = bump;
    v4qi v = {1, 2, 3, 4};

    OUT[0] = mix(3, 0x100, 0x10000UL, a) == 0x10105UL && sum(a, 5) == 260;
    OUT[1] = bump(1000, 7) == 1021 && sum(a + 3, 2) == 254;
    OUT[2] = via_addr(41) == 42;
    OUT[3] = vsum(v) == 10;
    OUT[4] = fs(a, 3) == 6 && fb(1, 1) == 4 && outer(a, 3, 100) == 6 + 5 + 100;
    OUT[5] = framed(a, 4) == 10;
    /* 1 + 2 + 2*4 + 3 + 3*2 */
    OUT[6] = nonleaf(1, 2, 3, 4, 3) == 20 && nonleaf(100, 200, 300, 400, 0) == 1400;
    OUT[7] = 0xAA;
    return 0;
}
