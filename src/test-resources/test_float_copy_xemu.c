/* test_float_copy_xemu.c — the float copies cc45 leaves out or makes calls (#485)
 *
 * A float operation's result used once by the next one stays in __float_a
 * (its first operand, FNEG's) or goes straight into __float_b (the second);
 * a copy into a place that holds the value already is left out (also into
 * the next block, reached only from the one before); a file with many
 * copies between places outside the base page calls __f32cpy for them.
 * A float read or written through a pointer is one loop, straight into
 * __float_a / __float_b or the variable stored next; an int converted to a
 * float goes straight into the variable it is stored to; a result stored to
 * a variable and used again after is read from the variable (not when a
 * call or a store between could change it).
 * Each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 AA
 */

#include <stdlib.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

float a, b, c, d, e, r, s, t;
volatile int pick;
float arr[6] = { 1.0, 2.5, 4.0, 8.0, -3.0, 0.0 };
float *fp;
volatile int ix;
signed int iv;
long lv;

static void set(void) {
    a = 3.0; b = 4.0; c = 10.0; d = 0.5; e = 2.0;
}

static void bump(void) { r = r + 100.0; }

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
    /* 13: an element the first operand, a variable the second */
    ix = 2;
    r = arr[ix] - b;                     /* 4 - 3 = 1 */
    if (r == 1.0) RESULT(12) = 0x0D;
    /* 14: an element the second operand (not commutative) */
    r = c - arr[ix];                     /* 0.5 - 4 = -3.5 */
    if (r == -3.5) RESULT(13) = 0x0E;
    /* 15: two elements, then a comparison with an element */
    ix = 3;
    r = arr[ix] / arr[ix - 2];           /* 8 / 2.5 = 3.2 */
    if (arr[ix - 3] < arr[ix]) s = 1.0; else s = 0.0;
    if (r > 3.19 && r < 3.21 && s == 1.0) RESULT(14) = 0x0F;
    /* 16: an element negated, converted, stored to a variable */
    ix = 4;
    r = -arr[ix];                        /* 3 */
    iv = (int)arr[ix + -3];              /* (int)2.5 = 2 */
    s = arr[ix];                         /* -3 */
    if (r == 3.0 && iv == 2 && s == -3.0) RESULT(15) = 0x10;
    /* 17: results and variables stored through a pointer, read back */
    fp = &arr[5];
    *fp = a * e;                         /* 4 * 4 = 16 */
    fp[-5] = d;                          /* arr[0] = 10 */
    if (arr[5] == 16.0 && arr[0] == 10.0 && *fp + fp[-5] == 26.0) RESULT(16) = 0x11;
    /* 18: an int converted into a variable, and into an operation */
    iv = -7; lv = 100000;
    r = iv;
    s = lv;
    t = iv * 1.0 + lv;                   /* 99993 */
    if (r == -7.0 && s == 100000.0 && t == 99993.0) RESULT(17) = 0x12;
    /* 19: a converted int the second operand */
    iv = 3;
    r = c - iv;                          /* 0.5 - 3 = -2.5 */
    if (r == -2.5) RESULT(18) = 0x13;
    /* 20: an element a function's result is added to (a call between) */
    ix = 1;
    r = arr[ix] + (float)abs(-2);        /* 2.5 + 2 */
    if (r == 4.5) RESULT(19) = 0x14;
    /* 21: a result stored, then compared and used again (a FOR's NEXT) */
    a = 4.0; b = 3.0; c = 0.5;
    r = a + b;                           /* 7 */
    if (r > a) s = r * c; else s = 0.0;  /* 3.5 */
    if (r == 7.0 && s == 3.5) RESULT(20) = 0x15;
    /* 22: a call between that writes the variable: the value before it */
    r = a - b;                           /* 1 */
    t = r;
    bump();                              /* r = 101 */
    s = t + r;                           /* 102 */
    if (s == 102.0) RESULT(21) = 0x16;
    /* 23: a store between to the variable */
    r = a * b;                           /* 12 */
    s = r;
    r = c;
    t = s + r;                           /* 12.5 */
    if (t == 12.5) RESULT(22) = 0x17;
    /* 24: converted ints and a result stored through a pointer from __float_a */
    iv = -9; ix = 2;
    arr[ix] = iv;                        /* -9 */
    arr[ix + 1] = iv * 2;                /* -18 */
    fp = &arr[4];
    *fp = arr[ix] - c;                   /* -9.5 */
    if (arr[2] == -9.0 && arr[3] == -18.0 && arr[4] == -9.5) RESULT(23) = 0x18;
    RESULT(24) = 0xAA;
    return 0;
}
