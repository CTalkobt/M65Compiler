/* test_break_n_xemu.c — break N leaves N enclosing loops/switches (Issue #71)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] break 2 out of two nested for loops (the issue's example)
 *   [1] break 3 out of for / while / do-while; break 1 is break
 *   [2] a switch counts as a level: break 2 in a switch leaves the loop
 *   [3] break 2 from the inner loop only; the outer loop goes on
 *   [4] break 2 releases the VLAs of the loops it leaves
 *   [5] break 2 in an inlined function and in a nested function
 *   [6] repeat(N) (unrolled) is not a level: break 2 in it leaves two for loops
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

static unsigned char screen[25 * 40];

NOINLINE static unsigned int find(unsigned char target) {
    unsigned char x, y;
    for (y = 0; y < 25; y++) {
        for (x = 0; x < 40; x++) {
            if (screen[y * 40 + x] == target)
                break 2;
        }
    }
    return y * 40 + x;
}

NOINLINE static unsigned char three(unsigned char stop) {
    unsigned char i, j = 0, k, n = 0;
    for (i = 0; i < 10; i++) {
        j = 0;
        while (j < 10) {
            k = 0;
            do {
                n++;
                if (k == 1) break 1;           /* this do-while only */
                if (i == stop && j == 2) break 3;
                k++;
            } while (k < 10);
            j++;
        }
    }
    return i * 16 + j + (n == stop * 20 + 2 * 2 + 1 ? 0 : 0x80);
}

NOINLINE static unsigned char sw(unsigned char *v) {
    unsigned char i, s = 0;
    for (i = 0; i < 10; i++) {
        switch (v[i]) {
        case 1: s += 1; break;                 /* the switch */
        case 2: s += 10; break 2;              /* the switch and the loop */
        default: s += 100; break;
        }
    }
    return s + i;                              /* i where it stopped */
}

NOINLINE static unsigned char inner_only(void) {
    unsigned char a, b, c, n = 0;
    for (a = 0; a < 3; a++)
        for (b = 0; b < 5; b++)
            for (c = 0; c < 5; c++) {
                n++;
                if (c == 1) break 2;           /* c and b; a goes on */
            }
    return n;                                  /* 3 * 2 */
}

static void *vla_at[3];

NOINLINE static unsigned char vla(unsigned char len) {
    unsigned char a, b, c, n = 0;
    for (a = 0; a < 3; a++)
        for (b = 0; b < 4; b++) {
            char u[len];
            u[0] = b;
            for (c = 0; c < 4; c++) {
                char v[len];
                v[0] = c;
                if (c == 0) vla_at[a] = v;
                n += u[0] + v[0];
                if (c == 2) break 2;
            }
        }
    return n;                                  /* 3 * (0+1+2) */
}

static unsigned char in_loop(unsigned char lim) {
    unsigned char i, j, n = 0;
    for (i = 0; i < lim; i++)
        for (j = 0; j < lim; j++) { n++; if (j == 1) break 2; }
    return n;
}

NOINLINE static unsigned char inlined(void) {
    unsigned char t, s = 0;
    for (t = 0; t < 4; t++) s += in_loop(5);   /* each call 2 */
    return s;
}

NOINLINE static unsigned char nested(unsigned char lim) {
    unsigned char t, s = 0;
    unsigned char inner(void) {
        unsigned char i, j, n = 0;
        for (i = 0; i < lim; i++)
            for (j = 0; j < lim; j++) { n++; if (j == 2) break 2; }
        return n;
    }
    for (t = 0; t < 3; t++) s += inner();     /* each call 3 */
    return s;
}

NOINLINE static unsigned char rep(void) {
    unsigned char a, b, n = 0;
    for (a = 0; a < 5; a++)
        for (b = 0; b < 5; b++)
            repeat (2) {                       /* unrolled: not a level */
                n++;
                if (n == 7) break 2;
            }
    return n + a * 16;                         /* 7, a = 0 */
}

int main(void) {
    unsigned char v[10] = { 1, 1, 3, 1, 2, 1, 1, 1, 1, 1 };
    screen[3 * 40 + 7] = 9;
    screen[20 * 40 + 1] = 9;
    OUT[0] = find(9) == 3 * 40 + 7 && find(5) == 25 * 40 + 40;
    OUT[1] = three(4) == 4 * 16 + 2;
    OUT[2] = sw(v) == 1 + 1 + 100 + 1 + 10 + 4;
    OUT[3] = inner_only() == 6;
    OUT[4] = vla(20) == 9 && vla_at[0] == vla_at[1] && vla_at[1] == vla_at[2];
    OUT[5] = inlined() == 8 && nested(5) == 9;
    OUT[6] = rep() == 7;
    OUT[7] = 0xAA;
    return 0;
}
