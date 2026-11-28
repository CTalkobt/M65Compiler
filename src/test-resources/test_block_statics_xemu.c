/* test_block_statics_xemu.c — block-scope statics of the same name in one
 * function are separate variables (#828: they were one symbol,
 * _f__local_g, the last initializer winning)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a table per case of a switch (the issue's font: each letter its own g)
 *   [1] two counters n in sibling blocks, each keeping its own count
 *   [2] an inner block's static shadowing the outer one's
 *   [3] a static named g__2 beside two g's (the second g's name made unique)
 *   [4] a static in a loop body's block: one variable across the iterations
 *   [5] the same names in another function: its own
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static volatile char vA = 'A', vB = 'B', vC = 'C';

static unsigned char glyph(char c, unsigned char row) {
    switch (c) {
        case 'A': { static const unsigned char g[5] = {2, 5, 7, 5, 5}; return g[row]; }
        case 'B': { static const unsigned char g[5] = {6, 5, 6, 5, 6}; return g[row]; }
        case 'C': { static const unsigned char g[5] = {3, 4, 4, 4, 3}; return g[row]; }
    }
    return 0xFF;
}

static unsigned char counters(unsigned char which) {
    if (which) { static unsigned char n = 10; return ++n; }
    else       { static unsigned char n = 100; return ++n; }
}

static unsigned char shadow(void) {
    static unsigned char s = 1;
    unsigned char r;
    {
        static unsigned char s = 50;
        s += 2;
        r = s;
    }
    s += 1;
    return (unsigned char)(r + s);   /* 52 + 2, then 54 + 3 */
}

static unsigned char names(void) {
    static unsigned char g__2 = 7;
    unsigned char t = 0;
    { static unsigned char g = 20; t += g; }
    { static unsigned char g = 30; t += g; }
    return (unsigned char)(t + g__2);   /* 57 */
}

static unsigned char loop(void) {
    unsigned char i, last = 0;
    for (i = 0; i < 4; i++) {
        static unsigned char k = 5;
        k += i;
        last = k;
    }
    return last;   /* 5 + 0 + 1 + 2 + 3 = 11 */
}

static unsigned char other(void) {
    { static unsigned char n = 3; n++; }
    { static unsigned char n = 40; return n; }
}

int main(void) {
    unsigned char i, ok;
    for (i = 0; i < 8; i++) OUT[i] = 0;

    ok = glyph(vA, 0) == 2 && glyph(vA, 2) == 7 && glyph(vB, 0) == 6 && glyph(vB, 1) == 5 &&
         glyph(vC, 0) == 3 && glyph(vC, 1) == 4 && glyph('D', 0) == 0xFF;
    OUT[0] = ok ? 1 : 0;

    ok = counters(1) == 11 && counters(0) == 101 && counters(1) == 12 && counters(0) == 102;
    OUT[1] = ok ? 1 : 0;

    ok = shadow() == 54 && shadow() == 57;
    OUT[2] = ok ? 1 : 0;

    OUT[3] = names() == 57 ? 1 : 0;

    ok = loop() == 11;
    OUT[4] = ok ? 1 : 0;

    OUT[5] = other() == 40 && counters(1) == 13 ? 1 : 0;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
