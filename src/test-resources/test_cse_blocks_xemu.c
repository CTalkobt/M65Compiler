/* test_cse_blocks_xemu.c — CSE across basic blocks (Issue #294): a costly
 * value (multiply, division, element address) computed in a block that
 * dominates another is reused there, unless something on the way changes
 * an operand
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a * 7 before an if/else, again in both branches and after the join
 *   [1] an operand changed in one branch: recomputed after the join
 *   [2] a loop changing an operand: recomputed each round
 *   [3] a variable changed through a pointer between the blocks
 *   [4] a division reused in a dominated block; not when the divisor changes
 *   [5] a pure call reused across blocks; not across a store it may read
 *   [6] an element address through a switch
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned int calls;
unsigned int g = 2;
unsigned int arr[8] = { 10, 11, 12, 13, 14, 15, 16, 17 };

NOINLINE __pure unsigned int scaled(unsigned int x);
NOINLINE unsigned int scaled(unsigned int x) { calls++; return x * g; }

NOINLINE static unsigned int join(unsigned int a, unsigned char k) {
    unsigned int r = a * 7;
    if (k) r += a * 7;
    else r -= a * 7 - 1;
    return r + a * 7;
}

NOINLINE static unsigned int changed(unsigned int a, unsigned char k) {
    unsigned int r = a * 7;
    if (k) a = 3;
    return r + a * 7;
}

NOINLINE static unsigned int loop(unsigned int a, unsigned int b) {
    unsigned int s = a * b, i;
    for (i = 0; i < 3; i++) { s += a * b; a++; }
    return s;
}

NOINLINE static unsigned int through(unsigned char k) {
    unsigned int i = 1, *pi = &i;
    unsigned int t = arr[i];
    if (k) *pi = 5;
    return t + arr[i];
}

NOINLINE static unsigned int divide(unsigned int n, unsigned int d, unsigned char k) {
    unsigned int q = n / d;
    if (k) q += n / d;
    if (k > 1) d = 1;
    return q + n / d;
}

NOINLINE static unsigned int pure(unsigned int x, unsigned char k) {
    unsigned int r = scaled(x);
    if (k == 1) r += scaled(x);
    if (k == 2) { g = 10; r += scaled(x); }
    return r;
}

NOINLINE static unsigned int pick(unsigned char i, unsigned char k) {
    unsigned int r = arr[i];
    switch (k) {
    case 0: r += arr[i]; break;
    case 1: r -= 1; break;
    default: r += arr[i] + arr[i]; break;
    }
    return r;
}

int main(void) {
    unsigned char ok;
    OUT[0] = join(3, 1) == 63 && join(3, 0) == 22;
    OUT[1] = changed(5, 1) == 56 && changed(5, 0) == 70;
    /* s = 2*3 + 2*3 + 3*3 + 4*3 */
    OUT[2] = loop(2, 3) == 33;
    OUT[3] = through(1) == 11 + 15 && through(0) == 22;
    ok = divide(100, 7, 0) == 28 && divide(100, 7, 1) == 42 && divide(100, 7, 2) == 128;
    OUT[4] = ok;
    g = 2; calls = 0;
    ok = pure(4, 1) == 16 && calls == 1;
    g = 2; calls = 0;
    ok = ok && pure(4, 2) == 8 + 40 && calls == 2;
    g = 2;
    OUT[5] = ok;
    OUT[6] = pick(2, 0) == 24 && pick(2, 1) == 11 && pick(3, 5) == 39;
    OUT[7] = 0xAA;
    return 0;
}
