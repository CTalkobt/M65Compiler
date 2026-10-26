/* test_far_ptr_xemu.c — far pointers (#68): T __far *p holds a 28-bit
 * address in 4 bytes; *p, p[i], p->m read and write through it with 32-bit
 * indirect addressing. Data in bank 4 ($40000-$403FF) and in bank 0.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 bytes: p[i], *p, *(p + n)          +1 ints, a byte of one
 *   +2 struct members, an array member    +3 longs; a near array as far
 *   +4 op=, ++/-- of far objects           +5 far pointer ++, +=, -, compares
 *   +6 parameters, returns, *p++           +7 globals initialized (labels, constants)
 *   +8 arrays of far pointers, &p[i]       +9 conversions near <-> far, sizeof
 *   +10 enum, _Bool, fixed point; *f() of a function returning one
 *   +11 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct P { int x; char tag; long big; char name[4]; struct { char a, b; } in; };
struct Rec { char tag; char __far *data; int len; };
enum Color { RED, GREEN = 300 };
typedef int __far *ifp;

char buf[8] = {10, 20, 30, 40, 50, 60, 70, 80};
char __far *gfar = (char __far *)0x40100;
char __far *gnear = buf;
long after = 0x11223344L;
char __far *tab[3] = {buf, "xyz", buf + 2};
struct Rec rec = {'a', buf + 1, 3};
struct Rec recs[2] = {{'b', buf, 1}, {'c', (char __far *)0x40300, 2}};

static long sum(const char __far *p, int n) {
    long s = 0;
    while (n--) s += *p++;
    return s;
}
static char __far *advance(char __far *p, int n) { return p + n; }
static int __far *pick(int which) { return which ? (int __far *)0x40200 : 0; }
static char __far *third(void) { return gfar + 2; }

int main(void) {
    char __far *p = (char __far *)0x40000;
    int __far *ip = (int __far *)0x40010;
    struct P __far *sp = (struct P __far *)0x40020;
    long __far *lp = (long __far *)0x40040;
    char __far *nb = buf;
    char __far *f;
    char *np;
    ifp q;
    int i;

    for (i = 0; i < 8; i++) p[i] = i * 3;
    OUT[0] = p[5] == 15 && *p == 0 && *(p + 7) == 21 && 3[p] == 9;

    ip[0] = 1000; ip[1] = 2000; ip[2] = ip[0] + ip[1];
    OUT[1] = ip[2] == 3000 && *(char __far *)0x40014 == (3000 & 0xFF);

    sp->x = 1234; sp->tag = 'q'; sp->big = 100000L; sp->name[2] = 7; sp->in.b = 9;
    OUT[2] = sp->x == 1234 && sp->tag == 'q' && sp->big == 100000L && sp->name[2] == 7 &&
             (*sp).x == 1234 && sp->in.b == 9 && sp[0].in.b == 9;

    *lp = 0x12345678L; lp[1] = *lp + 1;
    OUT[3] = lp[1] == 0x12345679L && nb[3] == 40;

    p[0] += 5; p[1]++; ++p[2]; ip[0] -= 10; sp->x <<= 1; i = p[3]--;
    OUT[4] = p[0] == 5 && p[1] == 4 && p[2] == 7 && ip[0] == 990 && sp->x == 2468 && i == 9 && p[3] == 8;

    q = ip;
    q++; q += 1;
    OUT[5] = *q == 3000 && q - ip == 2 && (char __far *)q - (char __far *)ip == 4 && q > ip && q != ip;

    for (i = 0; i < 8; i++) gfar[i] = (char)(i + 1);
    f = advance(gfar, 3);
    q = pick(1);
    q[0] = -2;
    OUT[6] = sum(gfar, 8) == 36 && sum(buf, 4) == 100 && *f == 4 && f - gfar == 3 &&
             !pick(0) && *(int __far *)0x40200 == 0xFFFE;

    OUT[7] = (long)gnear == (long)(unsigned)buf && after == 0x11223344L && gnear[2] == 30 &&
             rec.data[0] == 20 && rec.len == 3 && recs[0].data[3] == 40 && (long)recs[1].data == 0x40300L;

    OUT[8] = tab[0][1] == 20 && tab[1][1] == 'y' && tab[2][0] == 30 && tab[2] - tab[0] == 2 &&
             &gfar[5] - gfar == 5 && *&gfar[5] == 6;

    np = (char *)gnear;
    OUT[9] = np == buf && (char __far *)np == gnear && sizeof(*ip) == 2 && sizeof(rec.data) == 4 &&
             sizeof(char __far *) == 4 && sizeof(*sp) == sizeof(struct P);

    {
        enum Color __far *ep = (enum Color __far *)0x40400;
        _Bool __far *bp = (_Bool __far *)0x40410;
        __fixed8_8 __far *xp = (__fixed8_8 __far *)0x40420;
        *ep = GREEN; *bp = 5; *xp = 1.5;
        OUT[10] = *ep == GREEN && *bp == 1 && *(char __far *)0x40410 == 1 && *xp == 1.5 &&
                  *(int __far *)0x40420 == 0x180 && *third() == 3 && third()[1] == 4;
    }

    OUT[11] = 0xAA;
    return 0;
}
