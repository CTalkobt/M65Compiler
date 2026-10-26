/* test_near_ptr_xemu.c — near pointers (#68): T __near *p is a 1-byte offset
 * into the program's base page (__zp variables), read and written base-page
 * indexed ($00,x); a cast (or an assignment) to a normal or far pointer adds
 * the base page.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 bytes: p[i], *p, *(p + n)          +1 ints
 *   +2 struct members, op=, a long        +3 parameters, *p++, returns, p - q
 *   +4 to a normal / far pointer          +5 ++, +=, sizeof
 *   +6 globals initialized with addresses  +7 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct ZS { char a; int b; };
__zp char za[4];
__zp int zi[2];
__zp struct ZS zs;
__zp long zl;
__zp char zb[3] = {5, 6, 7};
char __near *g = &zb[1];
char __near *gt[2] = {zb, zb + 2};
char after = 0x77;

static char sumn(char __near *p, char n) {
    char s = 0;
    while (n--) s += *p++;
    return s;
}
static char __near *second(void) { return &za[1]; }

int main(void) {
    char __near *p = &za[0];
    int __near *ip = (int __near *)&zi[0];
    struct ZS __near *sp = &zs;
    long __near *lp = &zl;
    char *n16;
    char __far *fp;

    p[0] = 1; p[1] = 2; *(p + 2) = 3; p[3] = 4;
    OUT[0] = za[0] == 1 && za[3] == 4 && p[2] == 3;

    ip[1] = 1000; *ip = ip[1] + 1;
    OUT[1] = zi[0] == 1001 && zi[1] == 1000;

    sp->a = 7; sp->b = 300; sp->b += 5;
    lp[0] = 0x12345678L;
    OUT[2] = zs.a == 7 && zs.b == 305 && zl == 0x12345678L && *lp == 0x12345678L;

    OUT[3] = sumn(za, 4) == 10 && *second() == 2 && second() - p == 1;

    n16 = (char *)p; fp = p;
    OUT[4] = n16 == &za[0] && fp[3] == 4 && *n16 == 1;

    p++; ++p; p += 1;
    OUT[5] = *p == 4 && (p - (char __near *)za) == 3 && sizeof(p) == 1 && sizeof(char __near *) == 1;

    OUT[6] = *g == 6 && after == 0x77 && *gt[1] == 7 && gt[0][1] == 6;

    OUT[7] = 0xAA;
    return 0;
}
