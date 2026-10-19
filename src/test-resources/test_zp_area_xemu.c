/* test_zp_area_xemu.c — the area of functions' base-page blocks starts
 * above the part of the register pool ($30-$7F) the program uses
 *
 * ca45 records in each object the end of the pool its instructions name
 * (cc45.bptop; .bptop states it), and ln45 places the blocks from the
 * largest up to $F1. test_xemu.sh links this file alone (the blocks start
 * below $80), with test_zp_area_clob.s45 (whose clobber_pool writes all of
 * $30-$7F: they start at $80), and with test_zp_area_rom.s45 (it names $7E
 * but says .bptop $30: they start where they did alone).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] pointers in a block kept across a call that writes the pool
 *   [1] the same two levels deep, in a loop
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

#ifdef CLOBBER
void clobber_pool(void);
#else
NOINLINE void clobber_pool(void) { }
#endif

unsigned char text[8] = {1, 2, 3, 4, 5, 6, 7, 8};

NOINLINE static unsigned char pair(const unsigned char *p, const unsigned char *q) {
    const unsigned char *a = p + 1, *b = q + 2;
    clobber_pool();
    return (unsigned char)(*a + *b);
}

NOINLINE static unsigned char walk(const unsigned char *p, unsigned char n) {
    const unsigned char *s = p, *e = p + n;
    unsigned char sum = 0;
    while (s != e) {
        sum = (unsigned char)(sum + pair(s, e - 3));
        s++;
    }
    clobber_pool();
    return (unsigned char)(sum + *(e - 1));
}

int main(void) {
    OUT[0] = pair(text, text) == 2 + 3 ? 1 : 0;
    /* pair(text+i, text+2) = text[i+1] + text[4] for i = 0..4, then text[4] */
    OUT[1] = walk(text, 5) == (2 + 3 + 4 + 5 + 6) + 5 * 5 + 5 ? 1 : 0;
    OUT[2] = 0xAA;
    return 0;
}
