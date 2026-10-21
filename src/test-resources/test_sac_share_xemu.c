/* test_sac_share_xemu.c — static-allocation locals never live at the same
 * time share their storage (#324); those live together keep theirs
 *
 * Values that live across calls (to test_sac_share_b.c) are kept in the
 * function's SAC block (built with -fno-zp-locals: not its base-page
 * block). Each
 * part checks values that must survive while others are written:
 *   [0] branches of an if / else chain, each with its own locals across calls
 *   [1] a loop: an accumulator and a counter live around the back edge
 *       while each pass's temporaries are written
 *   [2] a local whose address is passed out (its own storage), and locals
 *       written while it is still read through the pointer later
 *   [3] nested loops with values of the outer loop live across the inner one
 *   [4] the same function again (its locals start over: nothing carried)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

/* in test_sac_share_b.c: real calls, so values live across them are kept
   in the SAC block */
unsigned int id(unsigned int v);
unsigned int sum_of(unsigned int *p);

unsigned int chain(unsigned char c) {
    unsigned int r = 0;
    if (c == 1) {
        unsigned int a = id(10), b = id(20);
        unsigned int d = id(a + b);
        r = a * 3 + b + d;                       /* 30 + 20 + 30 = 80 */
    } else if (c == 2) {
        unsigned int x = id(7), y = id(x * 2), z = id(y + x);
        r = x + y + z;                           /* 7 + 14 + 21 = 42 */
    } else {
        unsigned int p = id(100), q = id(p / 4);
        r = p - q + id(q);                       /* 100 - 25 + 25 = 100 */
    }
    return r;
}

unsigned int loop(unsigned char n) {
    unsigned int acc = id(1), k;
    for (k = 0; k < n; k++) {
        unsigned int t = id(k * 3);
        unsigned int u = id(t + 1);
        acc += u - t + id(k);                    /* + 1 + k */
    }
    return acc;                                  /* 1 + n + n(n-1)/2 */
}

unsigned int addr(void) {
    unsigned int pair[2];
    unsigned int before = id(5), after;
    pair[0] = id(30);
    pair[1] = id(12);
    after = id(before + 1);
    return sum_of(pair) + before + after;        /* 42 + 5 + 6 = 53 */
}

unsigned int nest(void) {
    unsigned int i, j, outer, total = 0;
    for (i = 1; i <= 3; i++) {
        outer = id(i * 100);
        for (j = 0; j < 4; j++) {
            unsigned int w = id(j);
            total += id(w + 1);
        }
        total += outer;                          /* outer survives the inner loop */
    }
    return total;                                /* 3 * 10 + 600 = 630 */
}

int main(void) {
    OUT[0] = chain(1) == 80 && chain(2) == 42 && chain(3) == 100;
    OUT[1] = loop(0) == 1 && loop(5) == 1 + 5 + 10 && loop(20) == 1 + 20 + 190;
    OUT[2] = addr() == 53;
    OUT[3] = nest() == 630;
    OUT[4] = chain(2) == 42 && loop(5) == 16 && addr() == 53 && nest() == 630;
    OUT[5] = 0xAA;
    return 0;
}
