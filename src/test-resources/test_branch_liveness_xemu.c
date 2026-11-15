/* test_branch_liveness_xemu.c — ca45: base-page stores across branches
 *
 * ca45 drops a store to a function's base-page temporary when no path from
 * it reads the byte (branches inside the function followed both ways), and
 * keeps what the registers hold at a label that several forward branches
 * reach (a 16-bit compare's "beq @else / bcc @else"). A byte one path still
 * reads, or a register only one branch leaves set, must stay.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a 16-bit value compared, then used on one path only (three ranges)
 *   [1] a value kept across a loop's back edge, read after the loop
 *   [2] a temporary read on the path that branches away, not on the other
 *   [3] two branches to one label with different values in the registers
 *   [4] the clamp of #567 with its array's address computed
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int tab[8] = {5, 100, 101, 300, 65535, 0, 256, 99};
unsigned char idx;

unsigned int pick(unsigned char i) {
    unsigned int v = tab[i];
    if (v > 100) return v - 100;
    return v + 1;
}

unsigned int sum_until(unsigned char n, unsigned int limit) {
    unsigned int s = 0, last = 0;
    unsigned char i;
    for (i = 0; i < n; i++) {
        last = tab[i];
        if (last > limit) continue;
        s += last;
    }
    return s + last;
}

unsigned int one_path(unsigned char i, unsigned char sel) {
    unsigned int a = tab[i], b = tab[(unsigned char)(i + 1)];
    unsigned int t = a + b;
    if (sel) {
        if (a > 100) return t;
        return b;
    }
    return a;
}

unsigned int two_values(unsigned int v) {
    unsigned int r;
    if (v == 300) r = 7;
    else if (v < 300) r = v;
    else r = 9;
    return r + 1;
}

typedef struct { unsigned char alive, x, y; unsigned int goods[16]; } col_t;
col_t cols[8];
unsigned char g;

unsigned char take(unsigned char c) {
    col_t *col = &cols[c];
    return col->goods[g] > 100 ? 100 : (unsigned char)col->goods[g];
}

int main(void) {
    volatile unsigned char z = 0;
    unsigned char ok;

    ok = pick(z) == 6 && pick(z + 1) == 101 && pick(z + 2) == 1 && pick(z + 3) == 200 &&
         pick(z + 4) == 65435u && pick(z + 5) == 1 && pick(z + 6) == 156;
    OUT[0] = ok;

    /* 5 + 100 (101, 300, 65535 over the limit) + 0, last = 0 at n = 6 */
    ok = sum_until(z + 6, 100) == 105 && sum_until(z + 5, 100) == 105 + 65535u &&
         sum_until(z + 8, 65535u) == (unsigned int)(5 + 100 + 101 + 300 + 65535u + 0 + 256 + 99 + 99) &&
         sum_until(z, 5) == 0;
    OUT[1] = ok;

    ok = one_path(z + 2, 1) == 401 && one_path(z, 1) == 100 && one_path(z + 2, 0) == 101 &&
         one_path(z + 3, 1) == (unsigned int)(300 + 65535u);
    OUT[2] = ok;

    ok = two_values(300 + z) == 8 && two_values(z + 20) == 21 && two_values(301 + z) == 10 &&
         two_values(z) == 1 && two_values(65535u - z) == 10;
    OUT[3] = ok;

    g = 3 + z;
    cols[2].goods[3] = 99;
    cols[5].goods[3] = 100;
    cols[7].goods[3] = 101;
    cols[1].goods[3] = 0x1200;
    ok = take(z + 2) == 99 && take(z + 5) == 100 && take(z + 7) == 100 && take(z + 1) == 100 && take(z) == 0;
    OUT[4] = ok;

    OUT[5] = 0xAA;
    return 0;
}
