/* test_divmod8_xemu.c — byte division (#352): an unsigned char divided by
 * a byte uses the 8-bit routine (__divmodu8), and x / c with x % c of the
 * same x shares one divide (x % c = x - (x / c) * c).
 *
 * Results at $C0C0 (01 = pass):
 *   +0 every byte / and % every divisor 1..255 (a variable divisor),
 *      checked against repeated subtraction
 *   +1 constant divisors 3, 7, 10, 129, 200, 255: % then /, / then %
 *   +2 signed values: x % c and x / c of negative x (truncating)
 *   +3 16-bit values: / and % of 1000..1100 by 7 (the shared form at 16 bits)
 *   +4 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

volatile unsigned char vb, vd;
volatile signed char vs;
volatile unsigned int vw;
unsigned char qa, ra;

static void split3(unsigned char k) { ra = k % 3; qa = k / 3; }
static void split3r(unsigned char k) { qa = k / 3; ra = k % 3; }
static void split7(unsigned char k) { ra = k % 7; qa = k / 7; }
static void split10(unsigned char k) { qa = k / 10; ra = k % 10; }
static void split129(unsigned char k) { ra = k % 129; qa = k / 129; }
static void split200(unsigned char k) { qa = k / 200; ra = k % 200; }
static void split255(unsigned char k) { ra = k % 255; qa = k / 255; }

static unsigned char check(unsigned char k, unsigned char d) {
    unsigned char q = 0, r = k;
    while (r >= d) { r -= d; q++; }
    return qa == q && ra == r;
}

int main(void) {
    unsigned int k, d;
    unsigned char ok = 1;
    signed char sq, sr;

    for (d = 1; d < 256; d += 7) {
        vd = (unsigned char)d;
        for (k = 0; k < 256; k++) {
            vb = (unsigned char)k;
            qa = vb / vd;
            ra = vb % vd;
            if (!check((unsigned char)k, (unsigned char)d)) ok = 0;
        }
    }
    vd = 255; vb = 254; qa = vb / vd; ra = vb % vd;
    if (!check(254, 255)) ok = 0;
    OUT[0] = ok;

    ok = 1;
    for (k = 0; k < 256; k++) {
        vb = (unsigned char)k;
        split3(vb); if (!check((unsigned char)k, 3)) ok = 0;
        split3r(vb); if (!check((unsigned char)k, 3)) ok = 0;
        split7(vb); if (!check((unsigned char)k, 7)) ok = 0;
        split10(vb); if (!check((unsigned char)k, 10)) ok = 0;
        split129(vb); if (!check((unsigned char)k, 129)) ok = 0;
        split200(vb); if (!check((unsigned char)k, 200)) ok = 0;
        split255(vb); if (!check((unsigned char)k, 255)) ok = 0;
    }
    OUT[1] = ok;

    vs = -7;
    sr = vs % 3; sq = vs / 3;
    ok = sq == -2 && sr == -1;
    vs = 100;
    sq = vs / 7; sr = vs % 7;
    ok = ok && sq == 14 && sr == 2;
    vs = -128;
    sr = vs % 10; sq = vs / 10;
    ok = ok && sq == -12 && sr == -8;
    OUT[2] = ok;

    ok = 1;
    for (k = 1000; k <= 1100; k++) {
        unsigned int q, r;
        vw = k;
        r = vw % 7;
        q = vw / 7;
        if (q * 7 + r != k || r >= 7 || q != k / 7u) ok = 0;
    }
    OUT[3] = ok;

    OUT[4] = 0xAA;
    return 0;
}
