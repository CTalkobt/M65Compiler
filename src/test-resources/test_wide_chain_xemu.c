/* test_wide_chain_xemu.c — several wide integer operations in one
 * expression (#329: the inner result, a struct, made the outer operation
 * pointer arithmetic)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __int128: v + v + k returned, (a - b) * c, (a & m) | (b ^ m)
 *   [1] unary operators on chained results: -(b - a), ~(a | b), -(a * c) + a
 *   [2] comparisons of chained results: a + b == c, a * 2 + 1 > b
 *   [3] unsigned long long (__int64): the same forms
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned __int128 u128;
typedef unsigned long long u64;

static u128 chain(unsigned char k, u128 v) { return v + v + k; }
u128 gchain(unsigned char k, u128 v) { return v + v + k; }

static unsigned char lo(u128 v) { return *(unsigned char *)&v; }
static unsigned char lo64(u64 v) { return *(unsigned char *)&v; }

int main(void) {
    volatile unsigned char seven = 7;
    u128 a = 40, b = 15, c = 3, m = 0x0F;
    u64 x = 40, y = 15, z = 3;
    OUT[0] = lo(chain(1, 5)) == 11 && lo(gchain(2, 5)) == 12 && lo((a - b) * c) == 75 &&
             lo((a & m) | (b ^ m)) == ((40 & 15) | (15 ^ 15)) && lo(a + b + c + seven) == 65;
    OUT[1] = lo(-(b - a)) == 25 && lo(~(a | b)) == (unsigned char)~(40 | 15) && lo(-(a * c) + a) == (unsigned char)(40 - 120);
    OUT[2] = (a + b == 55) && (a * 2 + 1 > b) && !(a - b - c < c);
    OUT[3] = lo64(x + x + seven) == 87 && lo64((x - y) * z) == 75 &&
             (x + y == 55) && lo64(~(x | y)) == (unsigned char)~(40 | 15);
    OUT[4] = 0xAA;
    return 0;
}
