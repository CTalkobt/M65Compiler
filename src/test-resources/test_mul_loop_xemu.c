/* test_mul_loop_xemu.c — multiplies as calls of __mul16 / __mul32 outside
 * loops, inline inside them (Issue #317); test_xemu.sh builds it by default,
 * with -fmul-call and with -fno-mul-call, and checks which use calls
 *
 * Results at $C0C0 (01 = pass):
 *   [0] 16-bit variable * variable outside a loop (unsigned and signed)
 *   [1] constant factors outside a loop: cheap ones (* 3, * 10: shifts and
 *       adds) and ones that are a call (* 1000, * 12345, * 0x8001: the
 *       factor through Y while A/X hold the multiplicand), of a byte too
 *   [2] 32-bit multiply outside a loop
 *   [3] the same inside loops (inline): a running product and a table
 *   [4] a multiply whose result is used after the call (registers kept)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

volatile unsigned int va = 123, vb = 456;
volatile signed int sa = -321, sb = 77;
volatile unsigned char vc = 201;
volatile unsigned long la = 100000UL, lb = 3;

N static unsigned int mul_vv(unsigned int a, unsigned int b) { return a * b; }
N static signed int mul_ss(signed int a, signed int b) { return a * b; }
N static unsigned int mul_c(unsigned int a) { return a * 3 + a * 10; }
N static unsigned int mul_big(unsigned int a) { return a * 1000; }
N static unsigned int mul_odd(unsigned int a) { return a * 12345; }
N static unsigned int mul_8001(unsigned int a) { return a * 0x8001; }
N static unsigned int mul_byte(unsigned char c) { return c * 1000; }
N static unsigned long mul_l(unsigned long a, unsigned long b) { return a * b; }

N static unsigned int loop_prod(unsigned int a, unsigned char n) {
    unsigned int r = 1, i;
    for (i = 0; i < n; i++) r = r * a + i * 7;
    return r;
}
static unsigned int table[6];
N static void loop_table(unsigned int k) {
    unsigned char i;
    for (i = 0; i < 6; i++) table[i] = i * k * 1000;
}
N static unsigned int keep(unsigned int a, unsigned int b, unsigned int c) {
    unsigned int p = a * b;     /* the call must not lose c */
    return p + c;
}

int main(void) {
    unsigned int r;
    OUT[0] = mul_vv(va, vb) == (unsigned int)(123u * 456u) && mul_ss(sa, sb) == -24717;
    OUT[1] = mul_c(va) == 123u * 13u && mul_big(va) == (unsigned int)(123000UL & 0xFFFF) &&
             mul_odd(va) == (unsigned int)((123UL * 12345UL) & 0xFFFF) &&
             mul_8001(va) == (unsigned int)((123UL * 0x8001UL) & 0xFFFF) &&
             mul_byte(vc) == (unsigned int)(201000UL & 0xFFFF);
    OUT[2] = mul_l(la, lb) == 300000UL && mul_l(la, la) == (unsigned long)(100000ULL * 100000ULL);
    loop_table(3);
    r = loop_prod(3, 4);    /* r: 1, 3, 16, 62, 207 */
    OUT[3] = r == 207 && table[0] == 0 && table[5] == (unsigned int)(15000UL & 0xFFFF);
    OUT[4] = keep(va, vb, 1000) == (unsigned int)(123u * 456u + 1000u);
    OUT[5] = 0xAA;
    return 0;
}
