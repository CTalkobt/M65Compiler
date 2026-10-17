/* test_zp_globals_xemu.c — __zp globals in the program's base page (#67),
 * with test_zp_globals_b_xemu.c (a second module)
 *
 * ln45 places __zp variables at the top of $80-$E9 of the private base page;
 * their operands are base-page ones (2 bytes), an instruction without a
 * base-page mode uses the full address. Initial values are copied at startup.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] initial values (scalars, a long, a static) and a zero one
 *   [1] an array, indexed (base page,X and absolute,Y)
 *   [2] a pointer: stores and loads through it
 *   [3] an address taken and written through (another module)
 *   [4] a variable of the other module
 *   [5] read-modify-write (a long, a static)
 *   [6] a pointer and a struct initialized from addresses / an aggregate
 *   [7] a static __zp local keeps its value between calls
 *   [8] the address is in the program's base page, $80-$E9
 *   [9] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

__zp unsigned char zc;
__zp int zi = 1234;
__zp long zl = 0x12345678L;
__zp unsigned char zarr[6] = { 1, 2, 3, 4, 5, 6 };
__zp unsigned char *zptr;
static __zp unsigned char zs = 7;
unsigned char buf[8] = { 9, 8, 7, 6, 5, 4, 3, 2 };
__zp unsigned char *zinit = buf + 1;
struct P { unsigned char x; int y; };
__zp struct P zpt = { 3, 500 };

extern __zp int zother;          /* test_zp_globals_b_xemu.c */
extern __zp unsigned char ztab[4];
int bump_other(void);
void set_via(int *p, int v);

NOINLINE static unsigned char sum_x(unsigned char n) {
    unsigned char i, s = 0;
    for (i = 0; i < n; i++) s += zarr[i];
    return s;
}

NOINLINE static unsigned char sum_y(unsigned char n) {
    unsigned char s = 0;
    while (n) s += zarr[--n];
    return s;
}

NOINLINE static unsigned char counter(void) {
    static __zp unsigned char n = 10;
    return ++n;
}

int main(void) {
    unsigned char i;
    OUT[0] = zc == 0 && zi == 1234 && zl == 0x12345678L && zs == 7;
    OUT[1] = sum_x(6) == 21 && sum_y(4) == 10;
    zptr = buf;
    for (i = 0; i < 8; i++) zptr[i] = i * 3;
    OUT[2] = buf[7] == 21 && zptr[5] == 15;
    set_via(&zi, 99);
    OUT[3] = zi == 99;
    zother = 40;
    OUT[4] = bump_other() == 41 && zother == 41 && ztab[2] == 30;
    zl += 0x100;
    zs += zc + 1;
    OUT[5] = zl == 0x12345778L && zs == 8;
    OUT[6] = *zinit == 3 && zpt.x == 3 && zpt.y == 500 && ++zpt.y == 501;
    counter();
    OUT[7] = counter() == 12;
    OUT[8] = ((unsigned)&zi & 0xFF) >= 0x80 && ((unsigned)&zi & 0xFF) < 0xEA &&
             ((unsigned)&zother & 0xFF) >= 0x80 && ((unsigned)&zother & 0xFF) < 0xEA;
    OUT[9] = 0xAA;
    return 0;
}
