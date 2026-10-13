/* test_math_errors_xemu.c — math domain/range errors return and set errno
 *
 * The BASIC ROM stops the program on these (?ILLEGAL QUANTITY, ?OVERFLOW);
 * the library catches them. Results at $C0C0 (01 = pass):
 *   [0] sqrtf(-1): 0, EDOM; sqrtf(4) still 2 with errno untouched
 *   [1] logf(0): -FLT_MAX, ERANGE; logf(-1): 0, EDOM; log10f(0) returns
 *   [2] expf(100): FLT_MAX, ERANGE; expf(-1e38): 0, ERANGE; expf(88) exact
 *   [3] powf(10, 100): FLT_MAX, ERANGE; powf(-1, 0.5): 0, EDOM;
 *       powf(0, -1): FLT_MAX, ERANGE; powf(-2, 3) == -8
 *   [4] fmodf(1, 0): 0, EDOM
 *   [5] strtof: 1e40 -> FLT_MAX, ERANGE; 1e-45 -> 0, ERANGE; 1e30 exact
 *   [6] $AA sentinel
 */
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h>
#define OUT ((volatile unsigned char *)0xC0C0)

volatile float m1 = -1.0, z = 0.0, hundred = 100.0, h = 0.5, ten = 10.0, huge = 1e38;

static int near(float a, float b) { float q = a / b; return q > 0.99999 && q < 1.00001; }

int main(void) {
    float r;
    errno = 0; r = sqrtf(m1);
    OUT[0] = r == 0.0 && errno == EDOM;
    errno = 0; r = sqrtf(4.0);
    OUT[0] = OUT[0] && near(r, 2.0) && errno == 0;

    errno = 0; r = logf(z);
    OUT[1] = r < -1e38 && errno == ERANGE;
    errno = 0; r = logf(m1);
    OUT[1] = OUT[1] && r == 0.0 && errno == EDOM;
    errno = 0; r = log10f(z);
    OUT[1] = OUT[1] && r < -1e37 && errno == ERANGE;

    errno = 0; r = expf(hundred);
    OUT[2] = r > 1e38 && errno == ERANGE;
    errno = 0; r = expf(-huge);
    OUT[2] = OUT[2] && r == 0.0 && errno == ERANGE;
    errno = 0; r = expf(88.0);
    OUT[2] = OUT[2] && near(r, 1.6516363e38) && errno == 0;

    errno = 0; r = powf(ten, hundred);
    OUT[3] = r > 1e38 && errno == ERANGE;
    errno = 0; r = powf(m1, h);
    OUT[3] = OUT[3] && r == 0.0 && errno == EDOM;
    errno = 0; r = powf(z, m1);
    OUT[3] = OUT[3] && r > 1e38 && errno == ERANGE;
    errno = 0; r = powf(-2.0, 3.0);
    OUT[3] = OUT[3] && near(r, -8.0) && errno == 0;

    errno = 0; r = fmodf(1.0, z);
    OUT[4] = r == 0.0 && errno == EDOM;

    errno = 0; r = strtof("1e40", 0);
    OUT[5] = r > 1e38 && errno == ERANGE;
    errno = 0; r = strtof("1e-45", 0);
    OUT[5] = OUT[5] && r == 0.0 && errno == ERANGE;
    errno = 0; r = strtof("-1.5e30", 0);
    OUT[5] = OUT[5] && near(r, -1.5e30) && errno == 0;

    OUT[6] = 0xAA;
    return 0;
}
