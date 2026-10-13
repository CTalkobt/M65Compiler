/* test_unroll_xemu.c — loop unrolling (-floop-unrolling) keeps results.
 * test_xemu.sh builds it with -floop-unrolling (and without);
 * test_optimizations.sh checks with -Roptir which loops were unrolled.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] simple: a[i] += k for i < 4; the variable after the loop
 *   [1] continue and break (the variable's value after a break)
 *   [2] nested loops (the inner one unrolled), counting down, step 3
 *   [3] unsigned char loop variable, an if/else in the body
 *   [4] zero-trip loop; calls in the body keep their order
 *   [5] a loop too large to unroll (300 iterations) still runs right
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

/* room for the loops with continue/break/if to be unrolled too */
#pragma cc45 max_unroll_bytes(1024)

int a[8], m[3][4];
int order[8];
int norder;

static void note(int v) { order[norder++] = v; }

static int simple(void) {
    int i, ok;
    for (i = 0; i < 8; i++) a[i] = i;
    for (i = 0; i < 4; i++) a[i] = a[i] + 10;
    ok = i == 4;
    return ok && a[0] == 10 && a[3] == 13 && a[4] == 4;
}

static int cont_break(void) {
    int s = 0, i, j;
    for (i = 0; i < 6; i++) {
        if (i == 2) continue;
        s += i;                 /* 0 + 1 + 3 + 4 + 5 */
    }
    for (j = 0; j < 6; j++) {
        if (a[j] == 12) break;  /* a[2] */
    }
    return s == 13 && i == 6 && j == 2;
}

static int nested_down_step(void) {
    int s = 0, d = 0, t = 0;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            m[r][c] = r * 4 + c;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            s += m[r][c];       /* 0..11: 66 */
    for (int i = 5; i > 0; i--) d = d * 10 + i;      /* 54321 */
    for (int i = 1; i <= 10; i += 3) t += i;         /* 1 + 4 + 7 + 10 */
    return s == 66 && d == 54321 && t == 22;
}

static int char_if(void) {
    unsigned char c;
    int even = 0, odd = 0;
    for (c = 0; c < 5; c++) {
        if (c & 1) odd += c; else even += c;
    }
    return even == 6 && odd == 4 && c == 5;
}

static int zero_calls(void) {
    int n = 0, i;
    for (i = 3; i < 3; i++) n++;
    norder = 0;
    for (int k = 0; k < 3; k++) note(k * 2);
    return n == 0 && i == 3 && norder == 3 && order[0] == 0 && order[1] == 2 && order[2] == 4;
}

static int big(void) {
    int s = 0;
    for (int i = 0; i < 300; i++) s += i;   /* 44850 */
    return s == 44850;
}

int main(void) {
    OUT[0] = simple();
    OUT[1] = cont_break();
    OUT[2] = nested_down_step();
    OUT[3] = char_if();
    OUT[4] = zero_calls();
    OUT[5] = big();
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
