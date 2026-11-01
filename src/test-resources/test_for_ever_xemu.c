/* test_for_ever_xemu.c — for (;;) loops and *p = *p + 1 (Issue #385:
 * cc45 v1.0.4 left out the branch back of a for loop without a condition,
 * and overwrote the byte loaded from *p before adding to it)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] for (;;) { *p = *p + 1; ... break; }: the body runs again (6 times)
 *   [1] for (c = 0; ; c++) { *p = c; ... break; }: c counts, *p follows it
 *   [2] *p = *p + 1 outside a loop, a byte and an int through pointers
 *   [3] for (;;) left by return, with continue in it
 *   [4] for (;;) left by goto, nested in another for (;;)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

volatile unsigned char cell = 40;
volatile unsigned int wcell = 1000;
volatile unsigned char limit = 46;

N static unsigned char forever_inc(volatile unsigned char *p) {
    unsigned char runs = 0;
    for (;;) {
        *p = *p + 1;
        runs++;
        if (*p == limit) break;
    }
    return runs;
}

N static unsigned char forever_count(volatile unsigned char *p) {
    unsigned char c;
    for (c = 0; ; c++) {
        *p = c;
        if (c == 9) break;
    }
    return c;
}

N static void add_one(volatile unsigned char *p, volatile unsigned int *q) {
    *p = *p + 1;
    *q = *q + 1;
}

N static unsigned int forever_return(unsigned char n) {
    unsigned int sum = 0;
    unsigned char i = 0;
    for (;;) {
        i++;
        if (i & 1) continue;     /* odd: skipped */
        sum += i;
        if (i >= n) return sum;  /* 2 + 4 + ... + n */
    }
}

N static unsigned int forever_nested(void) {
    unsigned char i = 0, j;
    unsigned int steps = 0;
    for (;;) {
        for (j = 0; ; j++) {
            steps++;
            if (j == 2) break;
        }
        if (++i == 4) goto out;
    }
out:
    return steps;   /* 4 outer runs of 3 inner ones */
}

int main(void) {
    unsigned char r;
    r = forever_inc(&cell);
    OUT[0] = r == 6 && cell == 46;
    cell = 200;
    r = forever_count(&cell);
    OUT[1] = r == 9 && cell == 9;
    cell = 41;
    add_one(&cell, &wcell);
    OUT[2] = cell == 42 && wcell == 1001;
    OUT[3] = forever_return(10) == 30;
    OUT[4] = forever_nested() == 12;
    OUT[5] = 0xAA;
    return 0;
}
