/* test_known_bits_xemu.c — code built from the bits known to be 0 (#350):
 * byte-wise or/and/xor where one side's byte is 0, adds without the carries
 * that cannot happen (and with the ones that can), far stores of x + c.
 * Inputs come from volatile globals, so nothing folds at compile time.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 (unsigned)y << 9 | (unsigned)x << 2, all x, y (the high byte from both)
 *   +1 and / xor with known-zero bytes; (x << 8) & 0x00FF is 0
 *   +2 long: zext + $08300000, + $100, + byte (carries through every byte)
 *   +3 k * 4 + 0x1010, + 1..3 (no carry) and + 4..7 (a carry possible)
 *   +4 far stores of c + 1 .. c + 3 at a 4-aligned c, read back
 *   +5 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

volatile unsigned char vx, vy, vk;
volatile unsigned int vw;
volatile unsigned long vl;

static unsigned int addr16(unsigned char x, unsigned char y) { return (unsigned int)y << 9 | (unsigned int)x << 2; }
static unsigned long addr32(unsigned char x, unsigned char y) {
    return 0x8300000UL + ((unsigned int)y << 9 | (unsigned int)x << 2);
}

int main(void) {
    unsigned int x, y, ok;
    unsigned long l;

    ok = 1;
    for (y = 0; y < 128; y += 3)
        for (x = 0; x < 256; x += 7) {
            vx = (unsigned char)x; vy = (unsigned char)y;
            /* (expected through longs: x << 2 reaches bit 9, y << 9's lowest) */
            unsigned long e = (((unsigned long)y << 9) | ((unsigned long)x << 2)) & 0xFFFFUL;
            if (addr16(vx, vy) != (unsigned int)e) ok = 0;
            if (addr32(vx, vy) != 0x8300000UL + e) ok = 0;
        }
    OUT[0] = ok;

    vx = 0xA5; vy = 0x3C; vw = 0x1234;
    OUT[1] = (((unsigned int)vx << 8) & 0x00FFu) == 0 && (((unsigned int)vx << 8) ^ vy) == 0xA53C &&
             (((unsigned int)vy << 8) | vx) == 0x3CA5 && ((vw & 0xFF00u) | vx) == 0x12A5 &&
             (((unsigned int)vx << 4) & 0x0FF0u) == 0x0A50;

    vw = 0xFFFF;
    l = (unsigned long)vw + 0x8300000UL;
    ok = l == 0x830FFFFUL;
    vl = 0x1234FFUL;
    ok = ok && vl + 0x100UL == 0x1235FFUL && vl + 1 == 0x123500UL;
    vl = 0xFFFFFFUL;
    ok = ok && vl + (unsigned long)vx == 0x10000A4UL;
    OUT[2] = ok;

    ok = 1;
    for (x = 0; x < 256; x++) {
        unsigned int c;
        vk = (unsigned char)x;
        c = 0x1010 + (unsigned int)vk * 4;
        if (c + 1 != 0x1011 + x * 4 || c + 3 != 0x1013 + x * 4 || c + 4 != 0x1014 + x * 4 || c + 7 != 0x1017 + x * 4)
            ok = 0;
    }
    OUT[3] = ok;

    ok = 1;
    for (x = 0; x < 256; x += 5) {
        unsigned int __far *p = (unsigned int __far *)0x40000UL;
        unsigned int c;
        vk = (unsigned char)x;
        c = 0x10F0 + (unsigned int)vk * 4;
        p[0] = c; p[1] = c + 1; p[2] = c + 2; p[3] = c + 3;
        if (p[0] != c || p[1] != c + 1 || p[2] != c + 2 || p[3] != c + 3) ok = 0;
    }
    OUT[4] = ok;

    OUT[5] = 0xAA;
    return 0;
}
