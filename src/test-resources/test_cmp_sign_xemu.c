/* test_cmp_sign_xemu.c — signed compares with 0 (x < 0, x >= 0) as a test of
 * the sign bit (lda hi; eor #$80; asl a: C = not negative)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int x < 0 over the edge values
 *   [1] int x >= 0
 *   [2] long x < 0 / x >= 0 (the sign byte from Z)
 *   [3] in branches (if)
 *   [4] [5] variables in memory (globals)
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))
N static unsigned char lt(signed int x) { return x < 0; }
N static unsigned char ge(signed int x) { return x >= 0; }
N static unsigned char ltl(signed long x) { return x < 0; }
N static unsigned char gel(signed long x) { return x >= 0; }
N static unsigned char br(signed int x) { if (x < 0) return 1; return 2; }
N static unsigned char brl(signed long x) { if (x >= 0) return 3; return 4; }
static signed int gv; static signed long glv;
N static unsigned char mem(void) { return (gv < 0) + 2 * (glv >= 0); }
int main(void) {
    OUT[0] = lt(-1) == 1 && lt(0) == 0 && lt(1) == 0 && lt(-32768) == 1 && lt(32767) == 0 && lt(-256) == 1 && lt(255) == 0;
    OUT[1] = ge(-1) == 0 && ge(0) == 1 && ge(1) == 1 && ge(-32768) == 0 && ge(32767) == 1 && ge(256) == 1;
    OUT[2] = ltl(-1L) == 1 && ltl(0L) == 0 && ltl(0x7FFFFFFFL) == 0 && ltl(-65536L) == 1 && gel(65536L) == 1 && gel(-2L) == 0;
    OUT[3] = br(-5) == 1 && br(5) == 2 && br(0) == 2 && brl(0L) == 3 && brl(-70000L) == 4;
    gv = -3; glv = 70000L; OUT[4] = mem() == 3; gv = 3; glv = -1; OUT[5] = mem() == 0;
    OUT[6] = 0xAA;
    return 0;
}
