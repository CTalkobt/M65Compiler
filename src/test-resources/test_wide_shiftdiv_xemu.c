/* test_wide_shiftdiv_xemu.c — shifts and division of wide integers (#330):
 * operator<<, >> (arithmetic), / and % (signed, truncating toward zero) of
 * <intwide.h>'s structs, and the compound assignments
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __int128 shifts: << and >> by small and large counts, past the
 *       width (0), a negative value shifted right keeps its sign
 *   [1] __int128 division: 1000 / 7, 1000 % 7 with each sign combination
 *       (C's truncation), a large quotient (2^100 / 2^60), x / 0
 *   [2] long long (__int64): the same forms
 *   [3] compound assignments: <<=, >>=, /=, %=
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef __int128 s128;
typedef long long s64;

static unsigned char lo(s128 v) { return *(unsigned char *)&v; }

int main(void) {
    volatile unsigned char three = 3;
    s128 v = 0x1234, one = 1, big, n16 = -16;
    s128 p = 1000, m = -1000, d = 7, nd = -7, zero = 0;
    s64 x = 0x1234, xp = 1000, xm = -1000, xd = 7;
    unsigned char ok;

    ok = (v << 4) == 0x12340 && ((v << 4) >> 4) == v && ((v << 100) >> 100) == v &&
         (v << 128) == 0 && (n16 >> 2) == -4 && (n16 >> 200) == -1 && ((one << 127) >> 127) == -1 &&
         (v >> three) == 0x246;
    OUT[0] = ok;

    big = one << 100;
    ok = p / d == 142 && p % d == 6 && m / d == -142 && m % d == -6 &&
         p / nd == -142 && p % nd == 6 && m / nd == 142 && m % nd == -6 &&
         big / (one << 60) == (one << 40) && (big + 5) % (one << 60) == 5 &&
         p / zero == -1 && p % zero == p;
    OUT[1] = ok;

    ok = (x << 4) == 0x12340 && ((x << 20) >> 20) == x && (x << 64) == 0 &&
         xp / xd == 142 && xm % xd == -6 && xm / xd == -142 && lo((s128)0) == 0;
    OUT[2] = ok;

    v = 1000;
    v <<= 3;          /* 8000 */
    v >>= 1;          /* 4000 */
    v /= 7;           /* 571 */
    v %= 100;         /* 71 */
    x = 1000;
    x /= 3;           /* 333 */
    x <<= 2;          /* 1332 */
    OUT[3] = v == 71 && x == 1332;
    OUT[4] = 0xAA;
    return 0;
}
