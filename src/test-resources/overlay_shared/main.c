/* overlay_shared/main.c — several objects in one code overlay (#328): a1.c,
 * a2.c (C, #pragma cc45 overlay(a)) and a3.s45 (asm, .overlay a) share
 * overlay a; b.c is overlay b and calls into a
 *
 * Results at $C0C0 (01 = pass):
 *   [0] each function of overlay a, called from resident code
 *   [1] overlay b calling into overlay a (a1 + a2 + a3 = 1 + 2 + 4)
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int a1(void);
int a2(void);
int a3(void);
int b1(void);

int main(void) {
    OUT[0] = a1() == 1 && a2() == 2 && a3() == 4;
    OUT[1] = b1() == 107;
    OUT[2] = 0xAA;
    return 0;
}
