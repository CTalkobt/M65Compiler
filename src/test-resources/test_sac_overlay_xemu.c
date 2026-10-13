/* test_sac_overlay_xemu.c — ln45's SAC overlay
 *
 * Static-allocation (SAC) functions keep parameters and locals in BSS. ln45
 * lets functions that are never active at the same time (neither reaches
 * the other through calls) share that storage; functions whose address is
 * taken, and everything they call, keep their own.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] two sibling functions share storage, and both compute correctly
 *   [1] arguments that are results of sharing functions: f(a(1), b(2))
 *   [2] a callback through a pointer keeps the caller's locals intact
 *   [3] a call chain keeps every level's locals across the calls below
 *   [4] a callee shared by two siblings
 *   [5] a tail call whose arguments are the caller's locals
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINL __attribute__((noinline))

unsigned int *pa, *pb;

NOINL int sib_a(int x) {
    int q[6];
    int i;
    for (i = 0; i < 6; i++) q[i] = x + i;
    pa = (unsigned int *)q;
    return q[0] + q[5];             /* 2x + 5 */
}

NOINL int sib_b(int x) {
    int r[6];
    int i;
    for (i = 0; i < 6; i++) r[i] = x * i;
    pb = (unsigned int *)r;
    return r[1] + r[5];             /* 6x */
}

NOINL int pair(int u, int v) {
    int w[3];
    w[0] = u; w[1] = v; w[2] = u - v;
    return w[0] * 100 + w[1] + w[2];
}

/* [2] callback */
NOINL int cb_leaf(int v) {
    int t[4];
    t[0] = v; t[3] = v + 7;
    return t[0] + t[3];             /* 2v + 7 */
}
NOINL int cb(int v) {
    int s[4];
    s[0] = v * 2; s[3] = cb_leaf(v);
    return s[0] + s[3];             /* 4v + 7 */
}
NOINL int apply(int (*fn)(int), int x) {
    int keep[4];
    keep[0] = x + 1000; keep[3] = x + 2000;
    int r = fn(x);
    return (keep[0] == x + 1000 && keep[3] == x + 2000) ? r : -1;
}

/* [3] chain */
NOINL int d3(int x) {
    int z[5];
    int i;
    for (i = 0; i < 5; i++) z[i] = x ^ (i * 3);
    return z[4] - z[0];
}
NOINL int d2(int x) {
    int y[5];
    y[0] = x; y[4] = x * 3;
    int r = d3(x + 1);
    return (y[0] == x && y[4] == x * 3) ? r + y[4] : -1;
}
NOINL int d1(int x) {
    int w[5];
    w[0] = x; w[4] = x + 11;
    int r = d2(x + 1);
    return (w[0] == x && w[4] == x + 11) ? r + w[4] : -1;
}

/* [4] a callee of two siblings */
NOINL int shared(int x) {
    int m[4];
    m[0] = x; m[3] = x << 1;
    return m[0] + m[3];             /* 3x */
}
NOINL int s1(int x) {
    int k[4];
    k[0] = x; k[3] = x + 5;
    int r = shared(x);
    return (k[0] == x && k[3] == x + 5) ? r + 1 : -1;
}
NOINL int s2(int x) {
    int k[4];
    k[0] = x; k[3] = x + 9;
    int r = shared(x + 1);
    return (k[0] == x && k[3] == x + 9) ? r + 2 : -1;
}

/* [5] tail call */
NOINL int t2(int x, int y) {
    int z[4];
    z[0] = x; z[1] = y; z[2] = x - y; z[3] = 0;
    return z[0] * 10 + z[1] + z[2] + z[3];
}
NOINL int t1(int a, int b) {
    int u = a * 3, v = b * 5;
    return t2(v, u);
}

int main(void) {
    int ok;

    OUT[6] = 0xAA;

    /* [0] */
    ok = sib_a(10) == 25 && sib_b(10) == 60;
    /* the two arrays overlap: the storage is shared */
    ok = ok && pa < pb + 6 && pb < pa + 6;
    OUT[0] = ok ? 1 : 0;

    /* [1] 25*100 + 60 + (25-60) */
    OUT[1] = pair(sib_a(10), sib_b(10)) == 2500 + 60 - 35 ? 1 : 0;

    /* [2] */
    OUT[2] = apply(cb, 5) == 27 && apply(cb_leaf, 4) == 15 ? 1 : 0;

    /* [3] d3(6) = (6^12) - 6 = 10 - 6 = 4; d2(5): 4 + 15 = 19; d1(4): 19 + 15 = 34 */
    OUT[3] = d1(4) == 34 ? 1 : 0;

    /* [4] */
    OUT[4] = s1(4) == 13 && s2(4) == 17 ? 1 : 0;

    /* [5] t2(10, 6) = 100 + 6 + 4 */
    OUT[5] = t1(2, 2) == 110 ? 1 : 0;

    return 0;
}
