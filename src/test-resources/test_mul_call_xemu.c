/* test_mul_call_xemu.c — multiplication as calls of __mul16 / __mul32
 * (cc45 -fmul-call, also -Os; test_xemu.sh builds this with -Os)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] signed and unsigned 16-bit products (the sign does not matter for
 *       the low 16 bits)
 *   [1] 32-bit products
 *   [2] constants, chars, a 32-bit multiplicand in a stack frame (inline)
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))
N static signed int m16(signed int a, signed int b) { return a * b; }
N static unsigned u16(unsigned a, unsigned b) { return a * b; }
N static signed long m32(signed long a, signed long b) { return a * b; }
N static unsigned k(unsigned a) { return a * 10 + a * 3; }
N static unsigned char c8(unsigned char a, unsigned char b) { return (unsigned char)(a * b); }
#pragma cc45 recurse
N static signed long fr(signed long a, signed long b, unsigned char d) { signed long x = a, y = b; if (d) return fr(a, b, d - 1); return x * y; }
int main(void) {
    OUT[0] = m16(-3, 7) == -21 && m16(-300, -100) == 30000 && m16(123, -45) == -5535 && u16(300, 200) == 60000u;
    OUT[1] = m32(-3, 70000L) == -210000L && m32(-1000L, -1000L) == 1000000L && m32(65536L, 3L) == 196608L;
    OUT[2] = k(7) == 91 && c8(20, 13) == 4 && fr(-1000L, 70L, 2) == -70000L;
    OUT[3] = 0xAA;
    return 0;
}
