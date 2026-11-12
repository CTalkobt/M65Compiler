/* test_codegen_waste_xemu.c — code that #542-#545 made smaller still
 * computes the same: a 16-bit value from two bytes (#542), = / <> 0 without
 * cmp #0 (#543), an array + a 16-bit offset (#544), p[-k] through one
 * p - 256 (#545); and the dead loads / flag restores ca45 now drops.
 *
 * Results at $C0C0: one byte per check, 0 = pass; $AA after the last
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NCHECK 16

unsigned char tbl[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
unsigned char map[64 * 40];
unsigned char n;
enum { K_COLONIST, K_SOLDIER };
static unsigned char res[NCHECK];

unsigned int join(const unsigned char *p, unsigned char i) {
    return ((unsigned int)p[4] << 8) | tbl[i];
}
unsigned int join2(unsigned char hi, unsigned char lo) {
    return ((unsigned int)hi << 8) | lo;
}
void tests(unsigned char fight, const unsigned char *p) {
    n = 0;
    if (fight != 0) n |= 1;
    if (p[1] == K_COLONIST) n |= 2;
    if (fight) n |= 4;
    if (p[2] != K_COLONIST) n |= 8;
}
unsigned char at(unsigned char x, unsigned char y) {
    const unsigned char *p = map + (((unsigned int)y << 6) | x);
    return p[1] + p[64];
}
unsigned char *cell(unsigned char x, unsigned char y) {
    return map + (((unsigned int)y << 6) | x);
}
unsigned char before(const unsigned char *p) {
    return p[-1] + p[-64];
}
unsigned char edges(const unsigned char *p) {
    return p[-255] ^ p[-1] ^ p[-256] ^ p[-128];
}
unsigned int wbefore(const unsigned int *w) {
    return w[-1] + w[-127];
}
void store_before(unsigned char *p, unsigned char v) {
    p[-1] = v;
    p[-200] = v + 1;
    p++;
    p[-1] = v + 2;     /* (p changed: another p - 256) */
}
unsigned char sum_left(const unsigned char *p, unsigned char k) {
    unsigned char s = 0;
    while (k--) { s += p[-1] + p[-2]; p++; }
    return s;
}

int main(void) {
    static unsigned char buf[600];
    unsigned char k, f[3];
    unsigned int i;
    for (k = 0; k < NCHECK; k++) res[k] = 1;
    for (i = 0; i < sizeof buf; i++) buf[i] = (unsigned char)(i * 7 + 3);
    for (i = 0; i < sizeof map; i++) map[i] = (unsigned char)(i ^ (i >> 8));

    /* #542 */
    res[0] = join(buf, 3) != (((unsigned int)buf[4] << 8) | 0x44);
    res[1] = join2(0x12, 0x34) != 0x1234 || join2(0, 0xFF) != 0x00FF || join2(0xFF, 0) != 0xFF00;
    /* #543 */
    f[0] = 9; f[1] = K_COLONIST; f[2] = K_SOLDIER;
    tests(0, f);
    k = n;
    tests(5, f);
    res[2] = k != (2 | 8) || n != (1 | 2 | 4 | 8);
    f[1] = 3; f[2] = 0;
    tests(1, f);
    res[3] = n != (1 | 4);
    /* #544 */
    res[4] = at(5, 7) != (unsigned char)(map[7 * 64 + 5 + 1] + map[7 * 64 + 5 + 64]);
    res[5] = at(63, 38) != (unsigned char)(map[38 * 64 + 63 + 1] + map[38 * 64 + 63 + 64]);
    res[6] = cell(10, 20) != &map[20 * 64 + 10] || cell(0, 0) != map;
    /* #545 */
    res[7] = before(buf + 300) != (unsigned char)(buf[299] + buf[236]);
    res[8] = before(buf + 64) != (unsigned char)(buf[63] + buf[0]);
    res[9] = edges(buf + 400) != (unsigned char)(buf[145] ^ buf[399] ^ buf[144] ^ buf[272]);
    res[10] = wbefore((const unsigned int *)(buf + 300)) !=
              (unsigned int)(*(unsigned int *)(buf + 298) + *(unsigned int *)(buf + 46));
    store_before(buf + 500, 0x40);
    res[11] = buf[499] != 0x40 || buf[300] != 0x41 || buf[500] != 0x42;
    {
        unsigned char s = 0, j;
        for (j = 0; j < 10; j++) s += buf[100 + j - 1] + buf[100 + j - 2];
        res[12] = sum_left(buf + 100, 10) != s;
    }
    /* a page boundary under p - 256 (low byte 0) */
    res[13] = before(buf + (256 - ((unsigned int)buf & 0xFF)) + 64) !=
              (unsigned char)(buf[256 - ((unsigned int)buf & 0xFF) + 63] + buf[256 - ((unsigned int)buf & 0xFF)]);
    res[14] = 0;
    res[15] = 0;
    for (k = 0; k < NCHECK; k++) OUT[k] = res[k];
    OUT[NCHECK] = 0xAA;
    return 0;
}
