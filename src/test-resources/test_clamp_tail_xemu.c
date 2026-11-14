/* test_clamp_tail_xemu.c — the leftovers of #567 and #566
 *
 * #567: a ?: whose result is used as a byte, one arm a constant
 * (x > 100 ? 100 : (unsigned char)x), is computed in 8 bits; one used wider
 * is not.
 * #566: ca45 drops a BRA to the next instruction when other labels lie
 * between (if (k == 27) return; inside a loop).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the clamp as a byte: below, at and above the limit, a high byte set
 *   [1] a constant arm over 255 used as a byte (its low byte), both arms constants
 *   [2] the same ?: used as an int keeps 16 bits
 *   [3] the loop with an early return
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct { unsigned char alive, x, y; unsigned int goods[16]; } col_t;
col_t cols[8];
unsigned char g;

unsigned char take(unsigned char c) {
    col_t *col = &cols[c];
    return col->goods[g] > 100 ? 100 : (unsigned char)col->goods[g];
}

unsigned char low(unsigned int v) {
    return v > 9 ? 0x1234 : v;
}

unsigned char two(unsigned char c) {
    return c ? 0x155 : 0x2AA;
}

unsigned int wide(unsigned int v) {
    return v > 300 ? 300 : v;
}

unsigned int wide2(unsigned char c, unsigned int v) {
    unsigned int r = c ? 0x1234 : v;
    return r + 1;
}

typedef struct { unsigned char where, kind, ref, job, jt, jg, x, y; } unit_t;
unit_t units[8];
unsigned char keys[6] = { 1, 2, 3, 27, 4, 5 };
unsigned char kpos, joined;

unsigned char key(void) { return keys[kpos++]; }
void join(unsigned char u) { joined += u + 1; }

void run(void) {
    unsigned char u, k;
    for (;;) {
        k = key();
        u = k & 7;
        if (k == 27) return;
        if (units[u].where == 1) join(u);
        if (units[u].where != 2) continue;
        units[u].jt = 7;
    }
}

int main(void) {
    unsigned char ok;

    /* [0] */
    ok = 1;
    g = 3;
    cols[2].goods[3] = 99;  if (take(2) != 99) ok = 0;
    cols[2].goods[3] = 100; if (take(2) != 100) ok = 0;
    cols[2].goods[3] = 101; if (take(2) != 100) ok = 0;
    cols[2].goods[3] = 0x0105; if (take(2) != 100) ok = 0;
    cols[7].goods[15] = 0;  g = 15; if (take(7) != 0) ok = 0;
    OUT[0] = ok;

    /* [1] */
    ok = 1;
    if (low(5) != 5 || low(10) != 0x34 || low(0x0203) != 0x34) ok = 0;
    if (two(1) != 0x55 || two(0) != 0xAA) ok = 0;
    OUT[1] = ok;

    /* [2] */
    ok = 1;
    if (wide(299) != 299 || wide(301) != 300 || wide(0x1000) != 300) ok = 0;
    if (wide2(1, 7) != 0x1235 || wide2(0, 0x02FF) != 0x0300) ok = 0;
    OUT[2] = ok;

    /* [3] */
    ok = 1;
    units[1].where = 1;
    units[2].where = 2;
    units[3].where = 3;
    units[4].where = 2;
    run();
    if (kpos != 4 || joined != 2) ok = 0;
    if (units[2].jt != 7 || units[1].jt != 0 || units[3].jt != 0 || units[4].jt != 0) ok = 0;
    OUT[3] = ok;

    OUT[4] = 0xAA;
    return 0;
}
