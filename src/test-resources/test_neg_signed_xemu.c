/* test_neg_signed_xemu.c — -x, ~x and +x of a signed integer are signed
 * (#338: e > -d was compared unsigned)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's comparisons: e > -d, e >= -d, -d < e, e > (-d)
 *   [1] what already worked: e > 0 - d, e > -5, a cast, a variable
 *   [2] -d in >> (arithmetic), / and % (signed)
 *   [3] -d widened to long (sign-extended), and as a signed char's -c
 *   [4] ~d and +d keep the sign; -l of a long; unsigned operands stay
 *       unsigned (-u > 0 for u = 5 is $FFFB > 0)
 *   [5] a Bresenham step, as in the game the issue came from
 *   [6] an LL constant a 32-bit long cannot hold is a long long: ~d & it,
 *       a signed long & it are 64-bit (GCC's pr59101, which #338's fix
 *       exposed: ~a, now signed, sign-extends)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

signed int e, d, ndy;
signed char c;
signed long l, w;
long long q;
unsigned int u;

int main(void) {
    unsigned char ok;
    e = 2; d = 5;

    OUT[0] = (e > -d) && (e >= -d) && (-d < e) && (e > (-d)) && !(-d > e);

    ndy = -d;
    OUT[1] = (e > 0 - d) && (e > -5) && (e > (signed int)-d) && (e > ndy);

    ok = 1;
    if ((-d >> 1) != -3) ok = 0;            /* -5 >> 1: arithmetic, -3 */
    if (-d / 2 != -2 || -d % 2 != -1) ok = 0;
    if (-d / e != -2) ok = 0;
    OUT[2] = ok;

    ok = 1;
    w = -d;                                 /* -5 as a long */
    if (w != -5L) ok = 0;
    l = -d + 0L;
    if (l != -5L) ok = 0;
    c = 3;
    if (!(-c < 0) || -c != -3) ok = 0;
    OUT[3] = ok;

    ok = 1;
    if (!(~d < 0) || ~d != -6) ok = 0;      /* ~5 = -6 */
    if (!(+d > 0)) ok = 0;
    l = 70000L;
    if (!(-l < 0) || -l != -70000L) ok = 0;
    u = 5;
    if (!(-u > 0)) ok = 0;                  /* unsigned: $FFFB */
    OUT[4] = ok;

    {
        /* a line from (0,0) to (7,3): Bresenham's error terms */
        signed int dx = 7, dy = 3, err = dx - dy, e2, x = 0, y = 0;
        unsigned char steps = 0;
        while (!(x == 7 && y == 3) && steps < 20) {
            e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x++; }
            if (e2 < dx) { err += dx; y++; }
            steps++;
        }
        OUT[5] = x == 7 && y == 3 && steps < 20;
    }
    {
        signed int z = 0;
        signed long sl = -1;
        ok = (~z & 4102790424LL) > 0 && ((~z & 4102790424LL) > 0 | 6) == 7;
        q = ~z & 4102790424LL;
        if (q != 4102790424LL || q < 0) ok = 0;
        q = sl & 4102790424LL;
        if (q != 4102790424LL) ok = 0;
        OUT[6] = ok;
    }
    OUT[7] = 0xAA;
    return 0;
}
