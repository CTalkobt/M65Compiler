/* test_ucmp_zero_xemu.c — unsigned comparisons with 0 and 1
 *
 * cc45 folds them: x > 0, 0 < x, x >= 1 are x != 0; x <= 0, x < 1 are
 * x == 0; x >= 0 is true and x < 0 false. Each is checked for a zero and a
 * nonzero value (volatile, so nothing is known at compile time).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned char   [1] unsigned int   [2] unsigned long
 *   [3] in branches (if/while) rather than as values
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

#define CHECK(z, n) \
    ((z > 0) == 0 && (n > 0) == 1 && (0 < z) == 0 && (0 < n) == 1 && \
     (z >= 1) == 0 && (n >= 1) == 1 && (z <= 0) == 1 && (n <= 0) == 0 && \
     (z < 1) == 1 && (n < 1) == 0 && (0 >= z) == 1 && (0 >= n) == 0 && \
     (z >= 0) == 1 && (n >= 0) == 1 && (z < 0) == 0 && (n < 0) == 0 && \
     (0 <= n) == 1 && (0 > n) == 0)

volatile unsigned char c0 = 0, c1 = 0x80;
volatile unsigned int i0 = 0, i1 = 0x0100;
volatile unsigned long l0 = 0, l1 = 0x01000000UL;

int main(void) {
    unsigned char a0 = c0, a1 = c1;
    unsigned int b0 = i0, b1 = i1;
    unsigned long d0 = l0, d1 = l1;
    int n = 0;
    OUT[0] = CHECK(a0, a1);
    OUT[1] = CHECK(b0, b1);
    OUT[2] = CHECK(d0, d1);
    if (d1 > 0) n++;
    if (b0 > 0) n += 10;
    if (a1 >= 1) n++;
    while (b1 > 0) { b1 >>= 4; n++; }      /* 0x0100: 3 steps */
    OUT[3] = n == 5;
    OUT[4] = 0xAA;
    return 0;
}
