/* test_float_range_xemu.c — float overflow, literals, conversions
 *
 * Results at $C0C0 (01 = pass):
 *   [0] multiply overflow saturates to the largest float (sign kept)
 *   [1] products near the top of the range are exact (scaled path)
 *   [2] division by zero saturates; 0 / 0 is 0
 *   [3] quotients near the top of the range / overflowing quotients
 *   [4] add/sub overflow saturates; big - big is 0
 *   [5] literals: 1e38 is a float constant, hex float 0x1.fp1 == 3.875
 *   [6] call through a function pointer converts int arguments to double
 *   [7] byte-indexed loop over a pointer to a float parameter's bytes
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

volatile float z = 0.0, big = 1e38, e30 = 1e30, e8 = 1e8, em30 = 1e-30, e20 = 1e20, em10 = 1e-10,
               one = 1.0, h = 0.5;

static int near(float a, float b) { float d = a / b; return d > 0.999999 && d < 1.000001; }
static int sat(float a, int neg) {
    unsigned char *p = (unsigned char *)&a;
    return p[0] == 0xFF && p[4] == 0xFF && ((p[1] & 0x80) != 0) == neg;
}
static float takes(double a, int b) { return a * b; }
static unsigned char sum_bytes(float a) {
    unsigned char *p = (unsigned char *)&a, k, s = 0;
    for (k = 0; k < 5; k++) s += p[k];
    return s;
}

int main(void) {
    float (*fp)(double, int) = takes;
    float x = 0x1.fp1;
    OUT[0] = sat(big * big, 0) && sat(big * -big, 1);
    OUT[1] = near(e30 * e8, 1e38) && near(big * h, 5e37);
    OUT[2] = sat(one / z, 0) && sat(-one / z, 1) && z / z == 0.0;
    OUT[3] = sat(big / em30, 0) && near(e20 / em10, 1e30) && near(big / one, 1e38);
    OUT[4] = sat(big + big, 0) && big - big == 0.0 && near(big + one, 1e38) && sat(-big - big, 1) &&
             near(1.5e38 + -1e38, 5e37);
    OUT[5] = big > 9.9e37 && x == 3.875 && 1e2 == 100.0;
    OUT[6] = fp(33, 2) == 66.0;
    OUT[7] = sum_bytes(1.0) == 0x81;
    OUT[8] = 0xAA;
    return 0;
}
