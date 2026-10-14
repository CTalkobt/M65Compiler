/* test_divmod_xemu.c — division and remainder through the library routines
 * (lib/stdlib_asm/divmod.s45: __divs16 ... __modu32)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] signed 16-bit / and % with every sign combination
 *   [1] unsigned 16-bit and char / and %, constant divisors
 *   [2] 32-bit signed and unsigned / and %
 *   [3] a leaf function (its parameters stay in $10...) and a pointer in a
 *       ZP register: the routines leave them alone
 *   [4] a 32-bit dividend in a stack frame (#pragma cc45 recurse: inline)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

N static signed int ds(signed int a, signed int b) { return a / b; }
N static signed int ms(signed int a, signed int b) { return a % b; }
N static unsigned du(unsigned a, unsigned b) { return a / b; }
N static unsigned mu(unsigned a, unsigned b) { return a % b; }
N static signed long dl(signed long a, signed long b) { return a / b; }
N static signed long ml(signed long a, signed long b) { return a % b; }
N static unsigned long dul(unsigned long a, unsigned long b) { return a / b; }
N static unsigned long mol(unsigned long a, unsigned long b) { return a % b; }
N static unsigned char dc(unsigned char a, unsigned char b) { return (unsigned char)(a / b); }
N static unsigned dk(unsigned a) { return a / 7 + a % 10 + a / 1000; }

/* leaf: a, b, p stay in the parameter block; the divisions must not clobber it */
N static unsigned leaf(unsigned a, unsigned b, const unsigned char *p) {
    unsigned q = a / b, r = a % b;
    return q + r + p[0] + p[1] + a / 3 + b;
}

#pragma cc45 recurse
N static signed long framed(signed long a, signed long b, unsigned char depth) {
    signed long x = a, y = b;
    if (depth) return framed(a, b, depth - 1);
    return x / y + x % y;
}

static const unsigned char two[2] = {10, 20};

int main(void) {
    OUT[0] = ds(-100, 7) == -14 && ds(100, -7) == -14 && ds(-100, -7) == 14 && ds(100, 7) == 14 &&
             ms(-100, 7) == -2 && ms(100, -7) == 2 && ms(-100, -7) == -2 && ds(-32768, 2) == -16384;
    OUT[1] = du(60000u, 7) == 8571 && mu(60000u, 7) == 3 && dc(200, 9) == 22 && du(5, 9) == 0 &&
             dk(12345) == 1763 + 5 + 12;
    OUT[2] = dl(-1000000L, 7) == -142857L && ml(-1000000L, 7) == -1 && dl(70000L, -3) == -23333L &&
             dul(4000000000UL, 3) == 1333333333UL && mol(4000000000UL, 7) == 3UL;
    /* leaf(1000, 7, two): 142 + 6 + 10 + 20 + 333 + 7 */
    OUT[3] = leaf(1000, 7, two) == 518;
    /* framed(-1000000, 7): -142857 + -1 */
    OUT[4] = framed(-1000000L, 7L, 3) == -142858L;
    OUT[5] = 0xAA;
    return 0;
}
