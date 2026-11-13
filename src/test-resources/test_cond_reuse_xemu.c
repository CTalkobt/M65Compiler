/* test_cond_reuse_xemu.c — an arm of ?: reuses what its condition computed
 * and loaded (#567), and loads again when memory may have changed
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's clamp through a pointer assigned once: both arms
 *   [1] p[i] ? p[i] : dflt, a[i] < 0 ? -a[i] : a[i], a global clamped
 *   [2] an arm that changes the memory before it reads it again: a call, a
 *       store through another pointer to the same object, a store to the
 *       global by name
 *   [3] a pointer assigned once: the object written between two reads
 *       (through another pointer, by name, by a call); a pointer assigned
 *       twice
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct { unsigned char alive, x, y; unsigned int goods[16]; } col_t;
col_t cols[8];
unsigned char g;
signed int sv[4];
unsigned char bv[4];
unsigned int gw;

__attribute__((noinline)) unsigned char take(unsigned char c) {
    col_t *col = &cols[c];
    return col->goods[g] > 100 ? 100 : (unsigned char)col->goods[g];
}

__attribute__((noinline)) unsigned char pick(const unsigned char *p, unsigned char i, unsigned char dflt) {
    return p[i] ? p[i] : dflt;
}

__attribute__((noinline)) int mag(unsigned char i) {
    return sv[i] < 0 ? -sv[i] : sv[i];
}

__attribute__((noinline)) unsigned int clampw(void) {
    return gw > 1000 ? 1000 : gw;
}

__attribute__((noinline)) void bump(void) { gw += 5; cols[2].goods[3] += 7; }

__attribute__((noinline)) unsigned int after_call(void) {
    return gw > 10 ? (bump(), gw) : gw + 1;
}

__attribute__((noinline)) unsigned int after_store(unsigned int *p, unsigned int *q) {
    return *p > 3 ? (*q = 9, *p) : *p + 1;
}

__attribute__((noinline)) unsigned int after_named(unsigned char c) {
    col_t *col = &cols[c];
    return col->goods[3] > 3 ? (cols[c].goods[3] = 77, col->goods[3]) : col->goods[3];
}

__attribute__((noinline)) unsigned char once(unsigned char c, col_t *other) {
    col_t *col = &cols[c];
    unsigned int a, b, d, e;
    a = col->goods[1];
    other->goods[1] = a + 1;      /* other may be col */
    b = col->goods[1];
    cols[c].goods[1] = b + 10;
    d = col->goods[1];
    bump();                       /* cols[2].goods[3] += 7 */
    e = cols[2].goods[3] > 5 ? col->goods[3] : 0;
    return a == 5 && b == 6 && d == 16 && e == cols[2].goods[3];
}

__attribute__((noinline)) unsigned int twice(unsigned char c, unsigned char d) {
    col_t *col = &cols[c];
    unsigned int a = col->goods[0] > 2 ? col->goods[0] : 1;
    col = &cols[d];
    return a + (col->goods[0] > 2 ? col->goods[0] : 1);
}

int main(void) {
    unsigned char ok;
    unsigned int w;

    g = 4;
    cols[3].goods[4] = 300; cols[5].goods[4] = 42; cols[5].goods[5] = 100; cols[0].goods[4] = 0x0164;
    ok = take(3) == 100 && take(5) == 42 && take(0) == 100;
    g = 5;
    OUT[0] = ok && take(5) == 100 && take(3) == 0;

    bv[1] = 7;
    ok = pick(bv, 1, 9) == 7 && pick(bv, 2, 9) == 9;
    sv[0] = -300; sv[1] = 300; sv[2] = 0;
    ok = ok && mag(0) == 300 && mag(1) == 300 && mag(2) == 0;
    gw = 999;
    ok = ok && clampw() == 999;
    gw = 2000;
    OUT[1] = ok && clampw() == 1000;

    gw = 20;
    ok = after_call() == 25;      /* the arm reads gw after bump() */
    gw = 4;
    ok = ok && after_call() == 5;
    w = 5;
    ok = ok && after_store(&w, &w) == 9 && w == 9;
    w = 5;
    gw = 0;
    ok = ok && after_store(&w, &gw) == 5 && gw == 9;
    w = 2;
    ok = ok && after_store(&w, &w) == 3 && w == 2;
    cols[6].goods[3] = 50;
    ok = ok && after_named(6) == 77;
    cols[7].goods[3] = 1;
    OUT[2] = ok && after_named(7) == 1;

    cols[2].goods[1] = 5; cols[2].goods[3] = 0;
    ok = once(2, &cols[2]);
    cols[1].goods[0] = 30; cols[4].goods[0] = 2;
    OUT[3] = ok && twice(1, 4) == 31 && twice(4, 1) == 31 && twice(1, 1) == 60;

    OUT[4] = 0xAA;
    return 0;
}
