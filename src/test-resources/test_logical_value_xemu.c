#pragma cc45 no_zp_save
/* test_logical_value_xemu.c — && / || used as values must yield 0 or 1
 *
 * Expected at $C0A0: 01 00 01 00 01 00 01 01 AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0A0 + (i)))
volatile int g0 = 0, g5 = 5, g1 = 1;
static int band(int a, int b) { return a && b; }
static int bor(int a, int b) { return a || b; }
void main() {
    R(0) = band(1, 1);      /* 1 */
    R(1) = band(0, 1);      /* 0  (after a call that left 1) */
    R(2) = g1 && g5;        /* 1, not 5 */
    R(3) = bor(0, 0);       /* 0 */
    R(4) = bor(1, 0);       /* 1 */
    R(5) = bor(0, 0);       /* 0  (after a call that left 1) */
    R(6) = g0 || g5;        /* 1, not 5 */
    R(7) = (g1 && g0) || (g5 && g1);  /* 1 */
    R(8) = 0xAA;
}
