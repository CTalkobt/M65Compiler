/* test_narrow_demand_xemu.c — #547: an expression whose result is used only
 * through its low byte (or a few bits) computed in 8 bits, through >> k and
 * 16-bit operands; #549: an element of a small array by a byte index in a
 * loop (abs,y, not a hoisted pointer); #551: the same sum on both sides of
 * && and in the guarded body computed once; #550: (unsigned long)x << k of
 * a byte or an int built in the result's bytes. Each against a reference that
 * goes through volatile 16-bit variables (full width).
 *
 * Results at $C0C0: one byte per check, 0 = pass; $AA after the last
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NCHECK 10
static unsigned char res[NCHECK];

unsigned char wave(unsigned int tick, unsigned char x, unsigned char y) {
    return (unsigned char)(((tick + (x >> 2) * 13 + (y >> 1) * 7) >> 3) & 1);
}
unsigned char wave_ref(unsigned int tick, unsigned char x, unsigned char y) {
    volatile unsigned int s = tick + (unsigned int)(x >> 2) * 13 + (unsigned int)(y >> 1) * 7;
    return (unsigned char)((s >> 3) & 1);
}
/* bits above the low byte are needed: must stay 16-bit */
unsigned char high_bit(unsigned int a, unsigned char b) {
    return (unsigned char)(((a + b * 3) >> 9) & 1);
}
unsigned char high_bit_ref(unsigned int a, unsigned char b) {
    volatile unsigned int s = a + (unsigned int)b * 3;
    return (unsigned char)((s >> 9) & 1);
}
/* the low byte of a 16-bit sum shifted: bits 2..9 needed (two bytes) */
unsigned char mid(unsigned int a, unsigned int b) {
    return (unsigned char)((a + b) >> 2);
}
unsigned char mid_ref(unsigned int a, unsigned int b) {
    volatile unsigned int s = a + b;
    return (unsigned char)(s >> 2);
}
/* a mask after a shift within the byte */
unsigned char nib(unsigned int a, unsigned char c) {
    return (unsigned char)(((a ^ c) * 5 >> 4) & 0x0F);
}
unsigned char nib_ref(unsigned int a, unsigned char c) {
    volatile unsigned int s = (a ^ c) * 5;
    return (unsigned char)((s >> 4) & 0x0F);
}
/* a signed shift: the sign comes from the high byte */
unsigned char sgn(signed int a) {
    return (unsigned char)((a >> 3) & 0x7F);
}
unsigned char sgn_ref(signed int a) {
    volatile signed int s = a >> 3;
    return (unsigned char)(s & 0x7F);
}

signed int cloud_x[3] = {-2, 40, 78};
unsigned char seen[3][3];
void put(unsigned char x, unsigned char c) { seen[c][x % 3] += x; }
void clouds(void) {
    unsigned char i, t;
    for (i = 0; i < 3; ++i)
        for (t = 0; t < 3; ++t)
            if (cloud_x[i] + t >= 0 && cloud_x[i] + t < 80)
                put((unsigned char)(cloud_x[i] + t), t);
}
signed int px;
unsigned char hits;
void on_screen(unsigned char t) {
    if (px + t >= 0 && px + t < 80) ++hits;
}
unsigned char either(unsigned char a, unsigned char b) {
    unsigned int s = (unsigned int)a + b;
    if ((unsigned int)a + b < 10 || (unsigned int)a + b > 300) return (unsigned char)((unsigned int)a + b);
    return 0;
    (void)s;
}

/* #550: (unsigned long)byte / int << k built in the result's bytes */
#define SH(k) \
    unsigned long shb##k(unsigned char v) { return (unsigned long)v << k; } \
    unsigned long shi##k(unsigned int v) { return (unsigned long)v << k; }
SH(1)
SH(3)
SH(4)
SH(6)
SH(7)
SH(8)
SH(9)
SH(12)
SH(15)
SH(16)
SH(17)
SH(20)
SH(22)
SH(24)
SH(25)
SH(28)
SH(31)
unsigned long shref(unsigned long v, unsigned char k) {
    volatile unsigned long x = v;
    while (k--) x = x + x;
    return x;
}
static unsigned char shifts(void) {
    static const unsigned char bv[] = {0, 1, 0x80, 0xFF, 0x5A};
    static const unsigned int iv[] = {0, 1, 0x8000, 0xFFFF, 0x1234, 0xA5C3};
    unsigned char bad = 0, i;
    for (i = 0; i < sizeof bv; i++) {
        if (shb1(bv[i]) != shref(bv[i], 1)) bad++;
        if (shb3(bv[i]) != shref(bv[i], 3)) bad++;
        if (shb4(bv[i]) != shref(bv[i], 4)) bad++;
        if (shb6(bv[i]) != shref(bv[i], 6)) bad++;
        if (shb7(bv[i]) != shref(bv[i], 7)) bad++;
        if (shb8(bv[i]) != shref(bv[i], 8)) bad++;
        if (shb9(bv[i]) != shref(bv[i], 9)) bad++;
        if (shb12(bv[i]) != shref(bv[i], 12)) bad++;
        if (shb15(bv[i]) != shref(bv[i], 15)) bad++;
        if (shb16(bv[i]) != shref(bv[i], 16)) bad++;
        if (shb17(bv[i]) != shref(bv[i], 17)) bad++;
        if (shb20(bv[i]) != shref(bv[i], 20)) bad++;
        if (shb22(bv[i]) != shref(bv[i], 22)) bad++;
        if (shb24(bv[i]) != shref(bv[i], 24)) bad++;
        if (shb25(bv[i]) != shref(bv[i], 25)) bad++;
        if (shb28(bv[i]) != shref(bv[i], 28)) bad++;
        if (shb31(bv[i]) != shref(bv[i], 31)) bad++;
    }
    for (i = 0; i < sizeof iv / sizeof iv[0]; i++) {
        if (shi1(iv[i]) != shref(iv[i], 1)) bad++;
        if (shi3(iv[i]) != shref(iv[i], 3)) bad++;
        if (shi4(iv[i]) != shref(iv[i], 4)) bad++;
        if (shi6(iv[i]) != shref(iv[i], 6)) bad++;
        if (shi7(iv[i]) != shref(iv[i], 7)) bad++;
        if (shi8(iv[i]) != shref(iv[i], 8)) bad++;
        if (shi9(iv[i]) != shref(iv[i], 9)) bad++;
        if (shi12(iv[i]) != shref(iv[i], 12)) bad++;
        if (shi15(iv[i]) != shref(iv[i], 15)) bad++;
        if (shi16(iv[i]) != shref(iv[i], 16)) bad++;
        if (shi17(iv[i]) != shref(iv[i], 17)) bad++;
        if (shi20(iv[i]) != shref(iv[i], 20)) bad++;
        if (shi22(iv[i]) != shref(iv[i], 22)) bad++;
        if (shi24(iv[i]) != shref(iv[i], 24)) bad++;
        if (shi25(iv[i]) != shref(iv[i], 25)) bad++;
        if (shi28(iv[i]) != shref(iv[i], 28)) bad++;
        if (shi31(iv[i]) != shref(iv[i], 31)) bad++;
    }
    return bad;
}
/* the issue's shape: a far address from a byte and an int */
unsigned long got_dst, got_src;
void dma_copy_far(unsigned long dst, unsigned long src, unsigned int n) { got_dst = dst; got_src = src; (void)n; }
void bake(unsigned char slot, unsigned int ch) {
    dma_copy_far(0x5E000UL + (unsigned long)slot * 64, (unsigned long)ch * 64, 64);
}

int main(void) {
    unsigned int t;
    unsigned char k, x, y;
    for (k = 0; k < NCHECK; k++) res[k] = 0;
    for (t = 0; t < 300; t += 7)
        for (x = 0; x < 250; x += 23)
            for (y = 0; y < 250; y += 31)
                if (wave(t * 211, x, y) != wave_ref(t * 211, x, y)) res[0]++;
    for (t = 0; t < 2000; t += 37)
        for (x = 0; x < 250; x += 17) {
            if (high_bit(t * 31, x) != high_bit_ref(t * 31, x)) res[1]++;
            if (mid(t * 31, t + x * 300) != mid_ref(t * 31, t + x * 300)) res[2]++;
            if (nib(t * 31, x) != nib_ref(t * 31, x)) res[3]++;
            if (sgn((signed int)(t * 31) - 20000) != sgn_ref((signed int)(t * 31) - 20000)) res[4]++;
        }
    clouds();
    /* cloud -2: t=2 -> x 0; cloud 40: 40,41,42; cloud 78: 78,79 */
    res[5] = !(seen[2][0] == 0 + 42 && seen[0][1] == 40 && seen[1][2] == 41 && seen[0][0] == 78 && seen[1][1] == 79);
    hits = 0;
    for (px = -5; px < 90; px += 3)
        for (k = 0; k < 4; k++) on_screen(k);
    {
        unsigned char h = 0;
        signed int p;
        for (p = -5; p < 90; p += 3)
            for (k = 0; k < 4; k++) if (p + k >= 0 && p + k < 80) h++;
        res[6] = hits != h || h == 0;
    }
    res[7] = either(3, 4) != 7 || either(200, 150) != (unsigned char)350 || either(20, 30) != 0;
    res[8] = shifts();
    bake(200, 0xFFFF);
    res[9] = got_dst != 0x5E000UL + 200UL * 64 || got_src != 0xFFFFUL * 64;
    for (k = 0; k < NCHECK; k++) OUT[k] = res[k];
    OUT[NCHECK] = 0xAA;
    return 0;
}
