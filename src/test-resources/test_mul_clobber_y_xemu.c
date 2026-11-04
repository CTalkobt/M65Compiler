/* test_mul_clobber_y_xemu.c — a function whose multiply by a constant is a
 * call of __mul16y (the factor in Y) records Y among its clobbered
 * registers (.reg_clobbers), so a caller does not keep an index in Y across
 * the call (#129: tiny-AES's MixColumns at -O0, xtime's * 0x1b)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] s[i][3] ^= xtime(s[i][3]) on each row: the right bytes changed
 *   [1] a multiply by a constant in the callee, an indexed store after it
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned char st[4][4];

static unsigned char xtime(unsigned char x) { return (x << 1) ^ (((x >> 7) & 1) * 0x1b); }
static unsigned int times7(unsigned int v) { return v * 7; }

static void mix(st *s) {
    unsigned char i, tm;
    for (i = 0; i < 4; ++i) {
        tm = (*s)[i][3];
        tm = xtime(tm);
        (*s)[i][3] ^= tm;
    }
}

int main(void) {
    unsigned char a[16], want[16], k, ok;
    unsigned int w[5];
    for (k = 0; k < 16; k++) {
        unsigned char v = k * 29 + 7, x = (v << 1) ^ ((v & 0x80) ? 0x1b : 0);
        a[k] = v;
        want[k] = (k & 3) == 3 ? v ^ x : v;
    }
    mix((st *)a);
    ok = 1;
    for (k = 0; k < 16; k++) if (a[k] != want[k]) ok = 0;
    OUT[0] = ok;

    for (k = 0; k < 5; k++) w[k] = 0;
    for (k = 1; k < 5; k++) w[k] = times7(k) + w[k - 1];
    OUT[1] = w[4] == 70 && w[1] == 7 && w[0] == 0;
    OUT[2] = 0xAA;
    return 0;
}
