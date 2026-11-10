/* test_cmp_const_xemu.c — comparisons of two constants folded in the IR
 *
 * A comparison whose operands are both constants after inlining (a constant
 * argument meeting a constant in the inlined body) becomes a constant 0 / 1,
 * and a branch on it is decided: the operands at their type's width,
 * signed or unsigned as the comparison. Each check writes its number at
 * $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

volatile unsigned char sink;

static inline unsigned char geu256(unsigned int x) { return x >= 256u; }
static inline unsigned char lts(signed int x) { return x < 1; }
static inline unsigned char ltu8(unsigned char x) { return x < 200; }
static inline unsigned char lts8(signed char x) { return x < 0; }
static inline unsigned char gtl(signed long x) { return x > 70000L; }
static inline unsigned char eq640(unsigned int x) { return x == 640u; }
static inline unsigned char pick(unsigned char d) {
    unsigned char v;
    if ((unsigned char)(d - 1) >= 8) v = 7; else v = d - 1;
    return v;
}

int main(void) {
    int i;
    for (i = 0; i < 8; i++) RESULT(i) = 0;
    if (geu256(320) == 1 && geu256(255) == 0) RESULT(0) = 0x01;
    if (lts(-5) == 1 && lts(1) == 0 && lts(-32768) == 1) RESULT(1) = 0x02;
    if (ltu8(199) == 1 && ltu8(200) == 0 && ltu8(255) == 0) RESULT(2) = 0x03;
    if (lts8(-1) == 1 && lts8(127) == 0 && lts8(-128) == 1) RESULT(3) = 0x04;
    if (gtl(70001L) == 1 && gtl(-80000L) == 0) RESULT(4) = 0x05;
    if (eq640(640) == 1 && eq640(320) == 0) RESULT(5) = 0x06;
    if (pick(2) == 1 && pick(0) == 7 && pick(9) == 7 && pick(8) == 7) RESULT(6) = 0x07;
    RESULT(7) = 0xAA;
    return 0;
}
