/* test_intn_native_xemu.c — __int(N) / __uint(N) of 8, 16 and 32 bits are
 * the native signed/unsigned char, int and long (no wide-integer struct)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sizes: 1, 2, 4
 *   [1] __int(32) arithmetic, a parameter and a result (signed)
 *   [2] __uint(32) above 2^31, __int(16) negative, __uint(8) wraps
 *   [3] mixed with long: __int(32) and signed long are the same type
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

__int(32) a = -100000L;
__uint(32) b = 3000000000UL;
__int(16) c = -5;
__uint(8) d = 250;

static __int(32) mul(__int(32) x, __int(32) y) { return x * y; }

int main(void) {
    signed long *pl = &a;
    OUT[0] = sizeof(__int(32)) == 4 && sizeof(__uint(32)) == 4 && sizeof(__int(16)) == 2 &&
             sizeof(__uint(8)) == 1;
    OUT[1] = a < 0 && a / 1000 == -100 && mul(a, -3) == 300000L;
    OUT[2] = b > 2000000000UL && c < 0 && c * 2 == -10 && (__uint(8))(d + 10) == 4;
    *pl += 1;
    OUT[3] = a == -99999L;
    OUT[4] = 0xAA;
    return 0;
}
