/* test_zp_args_speed_xemu.c — parameters copied into static storage on entry from
 * another file (#359): through __zp_args / __sac_args and a list
 * (-fzp-args: -O2, -Os) or inline (-O3, -Ospeed, -fno-zp-args); built both
 * ways, both conventions (test_zp_args_speed_b.c has the functions)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] three parameters (int, int, char)
 *   [1] five (char, long, int, char, int), two read twice
 *   [2] one byte, and a result
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
extern unsigned long acc;
void f3(unsigned int x, unsigned int y, unsigned char c);
void f5(unsigned char a, unsigned long b, unsigned int c, unsigned char d, unsigned int e);
unsigned int f1(unsigned char a);

int main(void) {
    unsigned int i;
    acc = 0;
    for (i = 0; i < 100; i++) f3(i, 7, 3);
    OUT[0] = acc == 4950UL + 700 + 300;
    acc = 0;
    f5(9, 0x12345678UL, 1000, 200, 0xFFFF);
    OUT[1] = acc == 9 + 0x12345678UL + 1000 + 200 + 0xFFFF + 9 + 0x12345678UL;
    acc = 0;
    OUT[2] = f1(77) == 231 && acc == 77;
    OUT[3] = 0xAA;
    return 0;
}
