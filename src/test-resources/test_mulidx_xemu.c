/* test_mulidx_xemu.c — the code the second October 2026 size pass changes:
 * the hardware multiply / divide (operands stored from X/Y/Z, the result
 * loaded into them), __mul16y (a byte constant factor outside loops),
 * p[i] through a pointer in ZP with a byte index ((p),y), and zero-extended
 * byte temporaries added without widening them first (the expected values
 * computed outside cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] unsigned 16- and 32-bit multiply, divide and remainder by variables
 *       and constants
 *   [1] signed 16- and 32-bit multiply, divide and remainder (all sign
 *       combinations)
 *   [2] byte constant factors outside a loop (__mul16y), others in a loop
 *   [3] p[i] loads and stores, i a byte, across a page boundary
 *   [4] sums of zero-extended byte loads (carries into the high byte)
 *   [5] 01
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static volatile unsigned int v1234 = 1234, v56 = 56, v7 = 7, vFFFF = 0xFFFF;
static volatile unsigned long vl1 = 123456UL, vl2 = 789UL;
static volatile signed int vs[4] = {-300, 300, -7, 7};
static volatile signed long vsl[2] = {-100000L, 100000L};
static volatile unsigned char vb[4] = {200, 100, 255, 3};

static int test_unsigned(void) {
    unsigned int a = v1234, b = v56;
    unsigned long la = vl1, lb = vl2;
    if (a * b != 3568u || a / b != 22u || a % b != 2u) return 0;   /* 69104 & $FFFF */
    if (a * 10u != 12340u || a / 10u != 123u || a % 10u != 4u) return 0;
    if (vFFFF * v7 != 0xFFF9u || vFFFF / v7 != 9362u || vFFFF % v7 != 1u) return 0;
    if (la * lb != 97406784UL || la / lb != 156UL || la % lb != 372UL) return 0;
    if (la * 1000UL != 123456000UL || la / 1000UL != 123UL || la % 1000UL != 456UL) return 0;
    return 1;
}

static int test_signed(void) {
    static const signed int prod[4][4] = {
        {24464, -24464, 2100, -2100}, {-24464, 24464, -2100, 2100},
        {2100, -2100, 49, -49}, {-2100, 2100, -49, 49}};
    static const signed int quot[4][4] = {
        {1, -1, 42, -42}, {-1, 1, -42, 42}, {0, 0, 1, -1}, {0, 0, -1, 1}};
    static const signed int rem[4][4] = {
        {0, 0, -6, -6}, {0, 0, 6, 6}, {-7, -7, 0, 0}, {7, 7, 0, 0}};
    unsigned char i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++) {
            signed int a = vs[i], b = vs[j];
            if (a * b != prod[i][j] || a / b != quot[i][j] || a % b != rem[i][j]) return 0;
        }
    if (vsl[0] * vsl[1] != (signed long)0xABF41C00L) return 0;   /* -10^10 mod 2^32 */
    if (vsl[0] / 7L != -14285L || vsl[0] % 7L != -5L || vsl[1] / -7L != -14285L) return 0;
    if ((signed int)vs[0] * -3 != 900 || (signed int)vs[1] / -3 != -100) return 0;
    return 1;
}

static unsigned int times80(unsigned int y) { return y * 80u; }
static unsigned int times200(unsigned char y) { return y * 200u; }

static int test_const_factor(void) {
    unsigned int t = 0;
    unsigned char k;
    if (times80(v7) != 560u || times80(v1234) != 33184u) return 0;   /* 98720 & $FFFF */
    if (times200(vb[2]) != 51000u || times200(vb[3]) != 600u) return 0;
    if (v56 * 13u != 728u || v1234 * 255u != 52526u) return 0;      /* 314670 & $FFFF */
    for (k = 0; k < 4; k++) t += vb[k] * 80u;
    return t == 44640u;
}

static unsigned char page[300];

static unsigned char sum_bytes(const unsigned char *p, unsigned char n) {
    unsigned char i, s = 0;
    for (i = 0; i < n; i++) s += p[i];
    return s;
}

static void fill_bytes(unsigned char *p, unsigned char n, unsigned char v) {
    unsigned char i;
    for (i = 0; i < n; i++) p[i] = v + i;
}

static int test_byte_index(void) {
    unsigned int k;
    unsigned char *p = page + 40;
    unsigned char i = vb[2];
    for (k = 0; k < 300; k++) page[k] = 0;
    fill_bytes(p, 255, 1);                        /* page[40 .. 294] = 1 .. 255 */
    if (page[39] != 0 || page[40] != 1 || page[294] != 255 || page[295] != 0) return 0;
    if (sum_bytes(p, 255) != (unsigned char)32640u) return 0;
    p[i - 1] = 0x5A;                              /* page[294] */
    if (page[294] != 0x5A || p[i - 1] != 0x5A || p[vb[3]] != 4) return 0;
    return 1;
}

static unsigned int add_bytes(const unsigned char *p) {
    unsigned int s = p[0];
    s += p[1];
    s += p[2];
    return s + p[3];
}

static int test_zext(void) {
    unsigned char buf[4];
    unsigned char k;
    for (k = 0; k < 4; k++) buf[k] = vb[k];
    if (add_bytes(buf) != 558u) return 0;
    if ((unsigned int)vb[0] + vb[2] != 455u || (unsigned int)vb[2] - vb[0] != 55u) return 0;
    if ((unsigned int)vb[1] - vb[0] != 0xFF9Cu) return 0;
    return 1;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    OUT[0] = test_unsigned() ? 1 : 0;
    OUT[1] = test_signed() ? 1 : 0;
    OUT[2] = test_const_factor() ? 1 : 0;
    OUT[3] = test_byte_index() ? 1 : 0;
    OUT[4] = test_zext() ? 1 : 0;
    OUT[5] = 1;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
