/* test_float_cmpbool_xemu.c — a float comparison's value (0 / 1) by one call
 *
 * A float comparison whose result is a value (not a branch) calls
 * __float_eq / _ne / _lt / _le / _gt / _ge (__ffast_* with -ffast-float),
 * A = 1 if it holds, else 0, when the file has two or more of that kind.
 * Each comparison is checked with a < b, a = b and a > b, signs and zero
 * among them; each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 08 AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

float a, b;
unsigned char r[6], q[6];

/* each kind twice, so both are calls */
static void cmp2(void) {
    r[0] = a < b;  r[1] = a <= b; r[2] = a > b;
    r[3] = a >= b; r[4] = a == b; r[5] = a != b;
    q[0] = a < b;  q[1] = a <= b; q[2] = a > b;
    q[3] = a >= b; q[4] = a == b; q[5] = a != b;
}

/* the results for a < b, a == b, a > b: lt le gt ge eq ne */
static const unsigned char want[3][6] = {
    { 1, 1, 0, 0, 0, 1 },
    { 0, 1, 0, 1, 1, 0 },
    { 0, 0, 1, 1, 0, 1 },
};

static int check(float x, float y, int w) {
    int i;
    a = x; b = y;
    cmp2();
    for (i = 0; i < 6; i++)
        if (r[i] != want[w][i] || q[i] != want[w][i]) return 0;
    return 1;
}

int main(void) {
    int i;
    for (i = 0; i < 9; i++) RESULT(i) = 0;
    if (check(1.0, 2.0, 0)) RESULT(0) = 0x01;
    if (check(2.0, 2.0, 1)) RESULT(1) = 0x02;
    if (check(3.5, 2.0, 2)) RESULT(2) = 0x03;
    if (check(-3.0, -2.0, 0)) RESULT(3) = 0x04;
    if (check(-2.0, 1.0, 0)) RESULT(4) = 0x05;
    if (check(0.0, 0.0, 1)) RESULT(5) = 0x06;
    if (check(0.0, -1.5, 2)) RESULT(6) = 0x07;
    if (check(1e10, 1e-10, 2)) RESULT(7) = 0x08;
    RESULT(8) = 0xAA;
    return 0;
}
