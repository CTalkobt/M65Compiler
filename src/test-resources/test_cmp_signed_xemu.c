/* test_cmp_signed_xemu.c — signed 16-bit compares: < and >= by subtraction
 * (C = not less), <= and > against a constant as < / >= with the next one
 * (not 32767), the others through cmp.s16
 *
 * Results at $C0C0 (01 = pass):
 *   [0] all six comparisons against a variable over 24 x 24 edge values
 *       (-32768 ... 32767), checked against 32-bit compares
 *   [1] against constants, as values (100, -300, 32767, -32768, 300)
 *   [2] against constants in branches (if)
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))
typedef signed int si;
N static unsigned char t(si x, si y) {   /* all six against a variable */
    return (unsigned char)((x < y) | (x <= y) << 1 | (x > y) << 2 | (x >= y) << 3 | (x == y) << 4 | (x != y) << 5);
}
N static unsigned char k(si x) {         /* against constants */
    return (unsigned char)((x < 100) | (x <= 100) << 1 | (x > -300) << 2 | (x >= -300) << 3 | (x < 32767) << 4 |
                           (x <= 32767) << 5 | (x > -32768) << 6 | (x >= 300) << 7);
}
N static unsigned char kb(si x) { unsigned char r = 0; if (x < 7) r |= 1; if (x <= -7) r |= 2; if (x > 1000) r |= 4; if (x >= 256) r |= 8; return r; }
static si vals[] = {-32768, -32767, -1000, -301, -300, -299, -256, -7, -1, 0, 1, 6, 7, 99, 100, 101, 255, 256, 299, 300, 1000, 1001, 32766, 32767};
static unsigned char ref_t(si x, si y) { signed long X = x, Y = y; return (unsigned char)((X < Y) | (X <= Y) << 1 | (X > Y) << 2 | (X >= Y) << 3 | (X == Y) << 4 | (X != Y) << 5); }
int main(void) {
    unsigned i, j; unsigned char ok = 1, ok2 = 1, ok3 = 1;
    for (i = 0; i < sizeof vals / sizeof vals[0]; i++) {
        signed long x = vals[i];
        unsigned char e = (unsigned char)((x < 100) | (x <= 100) << 1 | (x > -300) << 2 | (x >= -300) << 3 | (x < 32767) << 4 | (x <= 32767) << 5 | (x > -32768) << 6 | (x >= 300) << 7);
        unsigned char eb = (unsigned char)((x < 7) | (x <= -7) << 1 | (x > 1000) << 2 | (x >= 256) << 3);
        if (k(vals[i]) != e) ok2 = 0;
        if (kb(vals[i]) != eb) ok3 = 0;
        for (j = 0; j < sizeof vals / sizeof vals[0]; j++) if (t(vals[i], vals[j]) != ref_t(vals[i], vals[j])) ok = 0;
    }
    OUT[0] = ok; OUT[1] = ok2; OUT[2] = ok3; OUT[3] = 0xAA;
    return 0;
}
