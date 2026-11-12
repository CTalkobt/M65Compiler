/* test_mul_shapes_xemu.c — #552: repeated multiply shapes of a file share a
 * routine: y * W + x (__rowcol_W) and i * K, an element of an array of
 * structs (__mulc_K); every byte value of the operands against references
 * computed through volatile 16-bit values.
 *
 * Results at $C0C0: one byte per check, 0 = pass; $AA after the last
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NCHECK 6
#define COLS 80
unsigned char scr[COLS * 25], clr[COLS * 25];
typedef struct { unsigned char a, b, c; unsigned int d; unsigned char e[19]; } u24_t;   /* 24 bytes */
u24_t units[40];
static unsigned char res[NCHECK];

void put(unsigned char x, unsigned char y, unsigned char ch, unsigned char c) {
    scr[(unsigned int)y * COLS + x] = ch;
    clr[(unsigned int)y * COLS + x] = c;
}
void paint(unsigned char x, unsigned char y, unsigned char c) { clr[(unsigned int)y * COLS + x] = c; }
unsigned char peek(unsigned char x, unsigned char y) { return scr[(unsigned int)y * COLS + x]; }
void invert(unsigned char x, unsigned char y) { clr[(unsigned int)y * COLS + x] |= 0x20; }
unsigned int rc(unsigned char x, unsigned char y) { return (unsigned int)y * 41 + x; }
unsigned int rc2(unsigned char x, unsigned char y) { return x + (unsigned int)y * 41; }
unsigned int rc_ref(unsigned char x, unsigned char y) { volatile unsigned int t = y; t = t * 41; return t + x; }
unsigned int m6(unsigned char i) { return (unsigned int)i * 6; }
unsigned int m6b(unsigned char i) { return 6 * (unsigned int)i + 1; }
unsigned char kind(unsigned char u) { return units[u].c; }
unsigned int dval(unsigned char u) { return units[u].d; }

int main(void) {
    unsigned int i;
    unsigned char x, y;
    for (x = 0; x < NCHECK; x++) res[x] = 0;
    for (y = 0; y < 25; y++)
        for (x = 0; x < COLS; x++) put(x, y, (unsigned char)(x + y), (unsigned char)(x ^ y));
    for (i = 0; i < COLS * 25; i++) {
        unsigned char yy = (unsigned char)(i / COLS), xx = (unsigned char)(i % COLS);
        if (scr[i] != (unsigned char)(xx + yy) || clr[i] != (unsigned char)(xx ^ yy)) res[0]++;
        if (peek(xx, yy) != scr[i]) res[0]++;
    }
    paint(79, 24, 7); invert(79, 24); invert(0, 0);
    res[1] = clr[COLS * 25 - 1] != (7 | 0x20) || clr[0] != (0 | 0x20);
    x = 0;
    do {
        y = 0;
        do {
            if (rc(x, y) != rc_ref(x, y) || rc2(x, y) != rc_ref(x, y)) res[2]++;
            y += 17;
        } while (y >= 17);
        x += 13;
    } while (x >= 13);
    if (rc(255, 255) != 255u * 41 + 255) res[2]++;
    x = 0;
    do {
        volatile unsigned int r = x; r *= 6;
        if (m6(x) != r || m6b(x) != r + 1) res[3]++;
    } while (++x);
    for (x = 0; x < 40; x++) { units[x].c = (unsigned char)(x * 3); units[x].d = x * 1000u; }
    for (x = 0; x < 40; x++) if (kind(x) != (unsigned char)(x * 3) || dval(x) != x * 1000u) res[4]++;
    for (x = 0; x < NCHECK; x++) OUT[x] = res[x];
    OUT[NCHECK] = 0xAA;
    return 0;
}
