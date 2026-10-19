/* test_load16_xemu.c — a 16-bit or pointer global read where its one use is
 * (the next instruction: an add, subtract, compare or store), not copied
 * into a temporary first
 *
 * Results at $C0C0 (01 = pass):
 *   [0] add and subtract, the global as either operand
 *   [1] compares: <, >=, ==, != against a local, and against 0
 *   [2] a store of one global into another; a pointer global stored
 *   [3] an add stored straight into a local (the fused add/store path)
 *   [4] a value read before its global changes keeps the old value
 *   [5] a pointer global plus an offset, compared with another pointer
 *   [6] p[2] through a pointer global (its offset folded into the load:
 *       the pointer must be in its ZP home, not read where it is)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned int ga = 1000, gb = 250, gc;
signed int gs = -5;
char text[] = "abcdef";
char *gp = text, *gq;

NOINLINE static unsigned int add1(unsigned int x) { return ga + x; }
NOINLINE static unsigned int add2(unsigned int x) { return x + gb; }
NOINLINE static unsigned int sub1(unsigned int x) { return ga - x; }
NOINLINE static unsigned int sub2(unsigned int x) { return x - gb; }
NOINLINE static unsigned char lt(unsigned int x) { return ga < x; }
NOINLINE static unsigned char ge(unsigned int x) { return x >= gb; }
NOINLINE static unsigned char eq(unsigned int x) { return ga == x; }
NOINLINE static unsigned char ne0(void) { return gc != 0; }
NOINLINE static unsigned char slt(signed int x) { return gs < x; }
NOINLINE static void copy(void) { gc = ga; gq = gp; }
NOINLINE static unsigned int fused(unsigned int x) { unsigned int r; r = ga + x; r = r + 1; return r; }
NOINLINE static unsigned int keep(void) { unsigned int old = ga; ga = 7; return old + ga; }
NOINLINE static char third(void) { return gp[2]; }
NOINLINE static unsigned char after(unsigned int k) { return gp + k > text + 3; }

int main(void) {
    OUT[0] = add1(24) == 1024 && add2(5) == 255 && sub1(1) == 999 && sub2(260) == 10;
    gc = 0;
    OUT[1] = lt(1001) && !lt(1000) && ge(250) && !ge(249) && eq(1000) && !eq(999) && !ne0() && slt(-4) && !slt(-5);
    copy();
    OUT[2] = gc == 1000 && gq == text && ne0();
    OUT[3] = fused(23) == 1024;
    OUT[4] = keep() == 1007 && ga == 7;
    OUT[5] = after(4) && !after(3);
    OUT[6] = third() == 'c';
    OUT[7] = 0xAA;
    return 0;
}
