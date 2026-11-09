/* test_float_copy_xemu.c — the float copies cc45 leaves out or makes calls (#485)
 *
 * A float operation's result used once by the next one stays in __float_a
 * (its first operand, FNEG's) or goes straight into __float_b (the second);
 * a copy into a place that holds the value already is left out (also into
 * the next block, reached only from the one before); a file with many
 * copies between places outside the base page calls __f32cpy for them.
 * Each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 08 09 0A 0B 0C AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

float a, b, c, d, e, r, s, t;
volatile int pick;

static void set(void) {
    a = 3.0; b = 4.0; c = 10.0; d = 0.5; e = 2.0;
}

int main(void) {
    set();
    /* 1: the result the first operand of the next operation */
    r = a * b + c;                       /* 22 */
    if (r == 22.0) RESULT(0) = 0x01;
    /* 2: the result the second operand (not commutative) */
    r = c - a * b;                       /* -2 */
    if (r == -2.0) RESULT(1) = 0x02;
    /* 3: a division by a result */
    r = c / (a + e);                     /* 2 */
    if (r == 2.0) RESULT(2) = 0x03;
    /* 4: the negation of a result */
    r = -(a + b);                        /* -7 */
    if (r == -7.0) RESULT(3) = 0x04;
    /* 5: a comparison with a result as its second operand */
    if (c < a * b) RESULT(4) = 0x05;     /* 10 < 12 */
    /* 6: a value times itself, then reused */
    r = a * a;                           /* 9 */
    s = r + r;                           /* 18 */
    if (r == 9.0 && s == 18.0) RESULT(5) = 0x06;
    /* 7: a variable stored, then read at once (no copy back in) */
    r = a + d;                           /* 3.5 */
    s = r * e;                           /* 7 */
    if (s == 7.0) RESULT(6) = 0x07;
    /* 8: a value kept across a branch the block before ends with */
    r = b * e;                           /* 8 */
    if (pick) r = r + 1.0;
    s = r - a;                           /* 5 */
    if (s == 5.0) RESULT(7) = 0x08;
    /* 9: the other way through the branch */
    pick = 1;
    r = b * e;
    if (pick) r = r + 1.0;               /* 9 */
    s = r - a;                           /* 6 */
    if (s == 6.0) RESULT(8) = 0x09;
    /* 10: chains of several operations */
    r = ((a + b) * (c - e)) / (d * e);   /* 56 */
    if (r == 56.0) RESULT(9) = 0x0A;
    /* 11: copies between variables (many: through __f32cpy) */
    t = a; a = b; b = t;
    t = c; c = d; d = t;
    s = a; r = s; e = r;
    if (a == 4.0 && b == 3.0 && c == 0.5 && d == 10.0 && e == 4.0 && t == 10.0) RESULT(10) = 0x0B;
    /* 12: an assignment of a variable to itself, then a use */
    a = a;
    r = a - b;                           /* 1 */
    if (r == 1.0) RESULT(11) = 0x0C;
    RESULT(12) = 0xAA;
    return 0;
}
