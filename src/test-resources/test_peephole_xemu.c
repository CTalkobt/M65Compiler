/* test_peephole_xemu.c — the code the October 2026 size passes change:
 * a global's member / constant-index element at its address, sign
 * extension (sxt.8), and / or / eor with constants byte by byte, 32-bit
 * shifts by bytes and bits, flag tests ca45 drops, loads that become
 * register transfers (the expected values computed outside cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] members of global structs and constant-index elements: stores and
 *       loads of 1, 2 and 4 bytes, read back through a pointer; a nested
 *       function's
 *   [1] signed char to int and long (-128, -1, 0, 1, 127), and branches
 *       on the result's sign
 *   [2] 16-bit and / or / eor with constants (identity, constant and
 *       changed bytes) and a test of the result
 *   [3] 32-bit and / or / eor with constants
 *   [4] unsigned 32-bit >> 9 ... 31, signed >> on a negative value, << 9 ... 30
 *   [5] zero tests after widened loads and stores (ca45's flag tests),
 *       16-bit temporaries reloaded into X (transfers)
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct rec { unsigned char a, b; unsigned int w; unsigned long l; signed char s; };
static struct rec r;
static unsigned char arr[10];
static unsigned int warr[6];
static struct { unsigned char ok, want, ncode, nops, depth; } cs;

static volatile unsigned int vin16 = 0xA5C3;
static volatile unsigned long vin32 = 0xDEADBEEFUL;
static volatile signed long vneg = (signed long)0xEDCBA987UL;
static volatile signed char vsc[5] = {-128, -1, 0, 1, 127};
static volatile unsigned char vzero = 0, vone = 1;

static int bytes_are(const void *p, const unsigned char *want, unsigned char n) {
    const unsigned char *q = (const unsigned char *)p;
    unsigned char i;
    for (i = 0; i < n; i++)
        if (q[i] != want[i]) return 0;
    return 1;
}

static int test_globals(void) {
    static const unsigned char want[9] = {7, 9, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12, 0xFE};
    static const unsigned char want_cs[5] = {1, 0, 0, 0, 2};
    void inner(void) { cs.ok = 1; cs.depth = 2; }
    r.a = 7;
    r.b = 9;
    r.w = 0x1234;
    r.l = 0x12345678UL;
    r.s = -2;
    if (!bytes_are(&r, want, 9)) return 0;
    if (r.w != 0x1234 || r.l != 0x12345678UL || r.b != 9 || r.s != -2) return 0;
    arr[3] = 5;
    arr[9] = 11;
    warr[4] = 0xBEEF;
    if (arr[3] != 5 || arr[9] != 11 || warr[4] != 0xBEEF) return 0;
    if (((unsigned char *)warr)[8] != 0xEF || ((unsigned char *)warr)[9] != 0xBE) return 0;
    cs.ncode = cs.nops = cs.depth = 0;
    inner();
    return bytes_are(&cs, want_cs, 5);
}

static int test_sext(void) {
    static const signed int wi[5] = {-128, -1, 0, 1, 127};
    unsigned char i, neg = 0;
    for (i = 0; i < 5; i++) {
        signed int x = vsc[i];
        signed long y = vsc[i];
        if (x != wi[i] || y != (signed long)wi[i]) return 0;
        if (x < 0) neg++;
        if ((unsigned int)x != (unsigned int)wi[i]) return 0;
    }
    return neg == 2;
}

static int test_mask16(void) {
    unsigned int v = vin16;
    if ((v & 0x00FF) != 0x00C3 || (v & 0xFF00) != 0xA500 || (v & 0x03FF) != 0x01C3) return 0;
    if ((v | 0xFF00) != 0xFFC3 || (v | 0x00FF) != 0xA5FF) return 0;
    if ((v ^ 0xFF00) != 0x5AC3 || (v ^ 0x00FF) != 0xA53C || (v & 0) != 0) return 0;
    if ((v & 0xFF00) == 0) return 0;
    if (((v & 0x00FF) | 0) != 0xC3) return 0;
    return 1;
}

static int test_mask32(void) {
    unsigned long l = vin32;
    if ((l & ~63UL) != 0xDEADBEC0UL || (l | 0x00FF0000UL) != 0xDEFFBEEFUL) return 0;
    if ((l ^ 0x80000000UL) != 0x5EADBEEFUL || (l & 0x0000FFFFUL) != 0xBEEFUL) return 0;
    return (l & 0xFFFF0000UL) == 0xDEAD0000UL;
}

static int test_shift(void) {
    unsigned long u = vin32;
    signed long s = vneg;
    if ((u >> 9) != 0x6F56DFUL || (u >> 12) != 0xDEADBUL || (u >> 15) != 0x1BD5BUL || (u >> 17) != 0x6F56UL)
        return 0;
    if ((u >> 20) != 0xDEAUL || (u >> 23) != 0x1BDUL || (u >> 25) != 0x6FUL || (u >> 28) != 0xDUL) return 0;
    if ((u >> 30) != 3UL || (u >> 31) != 1UL) return 0;
    if ((unsigned long)(s >> 9) != 0xFFF6E5D4UL || (unsigned long)(s >> 12) != 0xFFFEDCBAUL) return 0;
    if ((unsigned long)(s >> 15) != 0xFFFFDB97UL || (unsigned long)(s >> 17) != 0xFFFFF6E5UL) return 0;
    if ((unsigned long)(s >> 20) != 0xFFFFFEDCUL || (unsigned long)(s >> 23) != 0xFFFFFFDBUL) return 0;
    if ((unsigned long)(s >> 25) != 0xFFFFFFF6UL || (unsigned long)(s >> 28) != 0xFFFFFFFEUL) return 0;
    if ((unsigned long)(s >> 30) != 0xFFFFFFFFUL) return 0;
    if ((u << 9) != 0x5B7DDE00UL || (u << 12) != 0xDBEEF000UL || (u << 20) != 0xEEF00000UL) return 0;
    return (u << 26) == 0xBC000000UL && (u << 30) == 0xC0000000UL;
}

static unsigned int sum3(unsigned int a, unsigned int b, unsigned int c) {
    unsigned int t = a * 3u;
    t += b;
    return t + c;
}

static int test_flags(void) {
    unsigned int w;
    unsigned char hits = 0;
    w = vzero;
    if (w) return 0;
    w = vone;
    if (w) hits++;
    if (vzero) return 0;
    if (vone) hits++;
    if (sum3(0x100, 0x20, 3) != 0x323) return 0;
    if (sum3(0xFFFF, 1, 1) != 0xFFFF) return 0;
    return hits == 2;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    OUT[0] = test_globals() ? 1 : 0;
    OUT[1] = test_sext() ? 1 : 0;
    OUT[2] = test_mask16() ? 1 : 0;
    OUT[3] = test_mask32() ? 1 : 0;
    OUT[4] = test_shift() ? 1 : 0;
    OUT[5] = test_flags() ? 1 : 0;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
