/* test_mulconst_xemu.c — multiplication by constants and 16-bit shifts done
 * with shifts and adds (no hardware multiplier), array indexing with
 * element sizes that are not powers of two
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int * constant (2, 3, 4, 5, 6, 10, 12, 13, 20, 24, 40, 80, 64, 256,
 *       512, 1000) against a multiply by repeated addition, over many values
 *   [1] unsigned char * constant (the byte value zero-extended)
 *   [2] int << 2..15
 *   [3] arrays of 2-, 3-, 4-, 5-, 10-, 13-, 80-byte elements indexed by a
 *       char and by an int (an element's last byte)
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int slow_mul(unsigned int a, unsigned int b) {
    unsigned int r = 0;
    while (b) { if (b & 1) r += a; a += a; b >>= 1; }
    return r;
}

static volatile unsigned int vin[8] = { 0, 1, 7, 100, 255, 1234, 40000u, 65535u };
static volatile unsigned char vch[6] = { 0, 1, 9, 100, 200, 255 };

struct e3 { unsigned char a, b, c; };
struct e5 { unsigned char d[5]; };
struct e10 { unsigned char d[10]; };
struct e13 { unsigned char d[13]; };
struct e80 { unsigned char d[80]; };
static unsigned int a2[20];
static struct e3 a3[20];
static unsigned long a4[20];
static struct e5 a5[20];
static struct e10 a10[20];
static struct e13 a13[20];
static struct e80 a80[20];

int main(void) {
    unsigned char i, ok;
    ok = 1;
    for (i = 0; i < 8; i++) {
        unsigned int x = vin[i];
        if (x * 2 != slow_mul(x, 2)) ok = 0;
        if (x * 3 != slow_mul(x, 3)) ok = 0;
        if (x * 4 != slow_mul(x, 4)) ok = 0;
        if (x * 5 != slow_mul(x, 5)) ok = 0;
        if (x * 6 != slow_mul(x, 6)) ok = 0;
        if (x * 10 != slow_mul(x, 10)) ok = 0;
        if (x * 12 != slow_mul(x, 12)) ok = 0;
        if (x * 13 != slow_mul(x, 13)) ok = 0;
        if (x * 20 != slow_mul(x, 20)) ok = 0;
        if (x * 24 != slow_mul(x, 24)) ok = 0;
        if (x * 40 != slow_mul(x, 40)) ok = 0;
        if (x * 80 != slow_mul(x, 80)) ok = 0;
        if (x * 64 != slow_mul(x, 64)) ok = 0;
        if (x * 256 != slow_mul(x, 256)) ok = 0;
        if (x * 512 != slow_mul(x, 512)) ok = 0;
        if (x * 1000 != slow_mul(x, 1000)) ok = 0;
    }
    OUT[0] = ok;
    ok = 1;
    for (i = 0; i < 6; i++) {
        unsigned char c = vch[i];
        if ((unsigned int)c * 2 != slow_mul(c, 2)) ok = 0;
        if ((unsigned int)c * 3 != slow_mul(c, 3)) ok = 0;
        if ((unsigned int)c * 5 != slow_mul(c, 5)) ok = 0;
        if ((unsigned int)c * 10 != slow_mul(c, 10)) ok = 0;
        if ((unsigned int)c * 13 != slow_mul(c, 13)) ok = 0;
        if ((unsigned int)c * 80 != slow_mul(c, 80)) ok = 0;
        if ((unsigned int)c * 64 != slow_mul(c, 64)) ok = 0;
    }
    OUT[1] = ok;
    ok = 1;
    for (i = 0; i < 8; i++) {
        unsigned int x = vin[i];
        unsigned char k;
        for (k = 2; k < 16; k++) {
            unsigned int want = slow_mul(x, 1u << k);
            unsigned int got = 0;
            switch (k) {
                case 2: got = x << 2; break;  case 3: got = x << 3; break;
                case 4: got = x << 4; break;  case 5: got = x << 5; break;
                case 6: got = x << 6; break;  case 7: got = x << 7; break;
                case 8: got = x << 8; break;  case 9: got = x << 9; break;
                case 10: got = x << 10; break; case 11: got = x << 11; break;
                case 12: got = x << 12; break; case 13: got = x << 13; break;
                case 14: got = x << 14; break; case 15: got = x << 15; break;
            }
            if (got != want) ok = 0;
        }
    }
    OUT[2] = ok;
    ok = 1;
    {
        unsigned char ci;
        unsigned int ii;
        for (ci = 0; ci < 20; ci++) {
            a2[ci] = ci; a3[ci].c = ci; a4[ci] = ci; a5[ci].d[4] = ci;
            a10[ci].d[9] = ci; a13[ci].d[12] = ci; a80[ci].d[79] = ci;
        }
        for (ii = 0; ii < 20; ii++) {
            unsigned char *b = (unsigned char *)a80;
            if (a2[ii] != ii || a3[ii].c != ii || a4[ii] != ii || a5[ii].d[4] != ii ||
                a10[ii].d[9] != ii || a13[ii].d[12] != ii || a80[ii].d[79] != ii) ok = 0;
            if (b[ii * 80 + 79] != ii) ok = 0;
        }
        if ((unsigned char *)&a13[7] - (unsigned char *)a13 != 91) ok = 0;
        if ((unsigned char *)&a80[19] - (unsigned char *)a80 != 1520) ok = 0;
    }
    OUT[3] = ok;
    OUT[4] = 0xAA;
    return 0;
}
