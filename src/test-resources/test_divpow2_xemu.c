/* test_divpow2_xemu.c — division and remainder by powers of two without the
 * division routine: unsigned ones are shifts and masks, signed ones add the
 * bias 2^k - 1 to a negative value first (C rounds toward zero), char ones
 * are reduced too. Each against the division routine (a volatile divisor).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] signed int / and % by 2, 4, 8, 16, 256, 1024 (incl. -32768, -1,
 *       -2^k, -2^k +- 1)
 *   [1] unsigned int / and % by the same
 *   [2] signed long / and % by 2, 8, 256, 65536
 *   [3] unsigned char and signed char / and % by 2, 4, 16
 *   [4] int >> 2..15 and signed int >> 2..15 (the shifts behind them)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static volatile signed int d2 = 2, d4 = 4, d8 = 8, d16 = 16, d256 = 256, d1024 = 1024;
static volatile signed long l2 = 2, l8 = 8, l256 = 256, l65536 = 65536L;
static const signed int sv[] = { 0, 1, -1, 2, -2, 3, -3, 7, -7, 8, -8, 9, -9, 15, -15, 16, -16, 17, -17,
                                 255, -255, 256, -256, 257, -257, 1023, -1023, 1024, -1024, 1025, -1025,
                                 32767, -32767, -32768, 12345, -12345 };
static const signed long lv[] = { 0, 1, -1, 7, -7, 8, -8, 255, -256, 65535L, -65535L, 65536L, -65536L,
                                  65537L, -65537L, 2147483647L, -2147483647L, -2147483647L - 1, 123456789L,
                                  -123456789L };

int main(void) {
    unsigned char i, ok;
    ok = 1;
    for (i = 0; i < sizeof sv / sizeof sv[0]; i++) {
        signed int x = sv[i];
        if (x / 2 != x / d2 || x % 2 != x % d2) ok = 0;
        if (x / 4 != x / d4 || x % 4 != x % d4) ok = 0;
        if (x / 8 != x / d8 || x % 8 != x % d8) ok = 0;
        if (x / 16 != x / d16 || x % 16 != x % d16) ok = 0;
        if (x / 256 != x / d256 || x % 256 != x % d256) ok = 0;
        if (x / 1024 != x / d1024 || x % 1024 != x % d1024) ok = 0;
    }
    OUT[0] = ok;
    ok = 1;
    for (i = 0; i < sizeof sv / sizeof sv[0]; i++) {
        unsigned int x = (unsigned int)sv[i];
        unsigned int u2 = d2, u4 = d4, u16 = d16, u1024 = d1024;
        if (x / 2 != x / u2 || x % 2 != x % u2) ok = 0;
        if (x / 4 != x / u4 || x % 4 != x % u4) ok = 0;
        if (x / 16 != x / u16 || x % 16 != x % u16) ok = 0;
        if (x / 1024 != x / u1024 || x % 1024 != x % u1024) ok = 0;
    }
    OUT[1] = ok;
    ok = 1;
    for (i = 0; i < sizeof lv / sizeof lv[0]; i++) {
        signed long x = lv[i];
        if (x / 2 != x / l2 || x % 2 != x % l2) ok = 0;
        if (x / 8 != x / l8 || x % 8 != x % l8) ok = 0;
        if (x / 256 != x / l256 || x % 256 != x % l256) ok = 0;
        if (x / 65536L != x / l65536 || x % 65536L != x % l65536) ok = 0;
    }
    OUT[2] = ok;
    ok = 1;
    for (i = 0; i < sizeof sv / sizeof sv[0]; i++) {
        unsigned char c = (unsigned char)sv[i];
        signed char s = (signed char)sv[i];
        if (c / 2 != c / d2 || c % 2 != c % d2) ok = 0;
        if (c / 4 != c / d4 || c % 4 != c % d4) ok = 0;
        if (c / 16 != c / d16 || c % 16 != c % d16) ok = 0;
        if (s / 2 != s / d2 || s % 2 != s % d2) ok = 0;
        if (s / 4 != s / d4 || s % 4 != s % d4) ok = 0;
        if (s / 16 != s / d16 || s % 16 != s % d16) ok = 0;
    }
    OUT[3] = ok;
    ok = 1;
    for (i = 0; i < sizeof sv / sizeof sv[0]; i++) {
        unsigned int u = (unsigned int)sv[i];
        signed int x = sv[i];
        unsigned char k;
        for (k = 2; k < 16; k++) {
            volatile unsigned char vk = k;
            unsigned int ur = 0; signed int sr = 0;
            switch (k) {
                case 2: ur = u >> 2; sr = x >> 2; break;   case 3: ur = u >> 3; sr = x >> 3; break;
                case 4: ur = u >> 4; sr = x >> 4; break;   case 5: ur = u >> 5; sr = x >> 5; break;
                case 6: ur = u >> 6; sr = x >> 6; break;   case 7: ur = u >> 7; sr = x >> 7; break;
                case 8: ur = u >> 8; sr = x >> 8; break;   case 9: ur = u >> 9; sr = x >> 9; break;
                case 10: ur = u >> 10; sr = x >> 10; break; case 11: ur = u >> 11; sr = x >> 11; break;
                case 12: ur = u >> 12; sr = x >> 12; break; case 13: ur = u >> 13; sr = x >> 13; break;
                case 14: ur = u >> 14; sr = x >> 14; break; case 15: ur = u >> 15; sr = x >> 15; break;
            }
            if (ur != u >> vk || sr != x >> vk) ok = 0;
        }
    }
    OUT[4] = ok;
    OUT[5] = 0xAA;
    return 0;
}
