/* test_codesize_xemu.c — the code-size changes of Issues #290, #292, #293, #295
 *
 * Results at $C0C0 (01 = pass):
 *   [0] constant arguments (char and int) stored straight into the
 *       parameter block, mixed with computed ones (#290); character
 *       constants are PETSCII ('c' is $43), the same on both sides
 *   [1] a ternary computed into its result: x ? a - b : c, nested (#293)
 *   [2] a temp stored to a global and overwritten (dead pool stores, #293)
 *   [3] tests of a long known on the path; a test after the variable
 *       changed is not folded (#295)
 *   [4] parameters used in place where no call has run, and copied where
 *       a call ran before a use (a later block, a loop with a call) (#292)
 *   [5] 16/32-bit zero tests of variables in memory (#295)
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned char g8;
unsigned int g16, g16b;
unsigned long g32;

NOINLINE static unsigned int args3(unsigned char a, unsigned char b, unsigned int n) {
    return a * 256 + b + n;
}
NOINLINE static unsigned int argsmix(unsigned int n, unsigned char c, unsigned long l) {
    return n + c + (unsigned int)(l >> 16);
}

NOINLINE static unsigned int tern(unsigned int l, unsigned int at, unsigned int n, unsigned char del) {
    if (!del) l += n;
    else l = l < at + n ? 0xFFFF : l - n;
    return l;
}
NOINLINE static unsigned int tern2(unsigned int a, unsigned int b) {
    unsigned int r = a > b ? (a > 100 ? a - 100 : a + 1) : (b == 0 ? 7 : b * 2);
    return r;
}

NOINLINE static void stores(unsigned int v) {
    g16 = v + 1;
    g16b = v + 2;
    g8 = (unsigned char)(v + 3);
}

static unsigned long probe;
NOINLINE static void touch(unsigned long *p) { *p += 1; }

NOINLINE static unsigned char known(unsigned long block, unsigned int len) {
    unsigned char r = 0;
    if (block) r |= 1;
    if (!block && !len) r |= 2;
    else if (!block || len > 5) r |= 4;
    if (block) r |= 8;
    block = 0;                       /* changed: the next test is not the earlier one */
    if (block) r |= 16;
    probe = len;
    touch(&probe);                   /* through a pointer: not a known variable */
    if (probe) r |= 32;
    return r;
}

static unsigned int calls;
NOINLINE static unsigned int bump(unsigned int x) { calls++; return x + 1; }

NOINLINE static unsigned int params(unsigned int a, unsigned int b, unsigned char k) {
    unsigned int s = 0;
    if (k == 1) s = a + b;           /* no call yet: a, b in place */
    else if (k == 2) s = a - b;
    s = bump(s);                     /* a call: the parameter block is reused */
    if (k == 3) s += a;              /* after the call: a must have been copied */
    return s;
}
NOINLINE static unsigned int loopcall(unsigned int a, unsigned char n) {
    unsigned int s = 0;
    while (n--) s += a + bump(0);    /* a used again after each call */
    return s;
}

NOINLINE static unsigned char zero16(unsigned int *p) { unsigned int v = *p; return v ? 1 : 0; }
NOINLINE static unsigned char zero32(unsigned long v, unsigned long w) {
    unsigned char r = 0;
    if (v) r |= 1;
    if (w) r |= 2;
    if (!v) r |= 4;
    return r;
}

int main(void) {
    unsigned int z = 0, nz = 0x100;
    /* (character constants are PETSCII: the expected values use the same ones) */
    OUT[0] = args3('c', 'l', 5) == 'c' * 256 + 'l' + 5 &&
             argsmix(1000, 'A', 0x00030000UL) == 1000 + 'A' + 3 &&
             args3((unsigned char)(nz >> 8), 2, nz) == 0x100 + 2 + 0x100;
    OUT[1] = tern(10, 5, 3, 0) == 13 && tern(6, 5, 3, 1) == 0xFFFF && tern(20, 5, 3, 1) == 17 &&
             tern2(150, 10) == 50 && tern2(50, 10) == 51 && tern2(5, 10) == 20 && tern2(0, 0) == 7;
    stores(0x1234);
    OUT[2] = g16 == 0x1235 && g16b == 0x1236 && g8 == 0x37;
    OUT[3] = known(0x10000UL, 0) == (1 | 8 | 32) && known(0, 0) == (2 | 32) &&
             known(0, 9) == (4 | 32) && known(0, 3) == (4 | 32) && known(1, 9) == (1 | 4 | 8 | 32);
    calls = 0;
    OUT[4] = params(10, 3, 1) == 14 && params(10, 3, 2) == 8 && params(10, 3, 3) == 11 &&
             loopcall(5, 3) == 3 * (5 + 1) && calls == 6;
    g32 = 0;
    OUT[5] = zero16(&z) == 0 && zero16(&nz) == 1 &&
             zero32(0, 0x01000000UL) == (2 | 4) && zero32(0x80, 0) == 1;
    OUT[6] = 0xAA;
    return 0;
}
