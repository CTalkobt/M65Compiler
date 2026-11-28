/* test_loop_regs_xemu.c — a loop's byte values kept in X, Y or Z (#827):
 * cc45 names each innermost loop's byte counters and values (.loop_vars),
 * ca45 keeps them in a register the loop's code leaves alone, loaded where
 * the loop is entered and stored before each way out (the expected values
 * computed outside cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] CRC-8 of a table: the crc and the bit counter of the inner loop
 *   [1] a search left by break: the counter read after the loop
 *   [2] nested loops: the inner counter, a count and a byte sum
 *   [3] a copy with an xor through a pointer, a byte accumulator
 *   [4] a count down by 3 (the counter and the remainder after it), and a
 *       loop with a call (left in memory)
 *   [5] continue: the sum of the even values
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char tab[64];
static unsigned char dst[64];
static volatile unsigned char vkey = 41, vmask = 0x5a, vstart = 200;

static unsigned char crc8(const unsigned char *data, unsigned int length) {
    unsigned char crc = 0, j;
    unsigned int i;
    for (i = 0; i < length; i++) {
        crc ^= *data++;
        for (j = 8; j; j--) {
            if ((crc & 0x80) != 0) crc = (crc << 1) ^ 0x1d;
            else crc <<= 1;
        }
    }
    return crc;
}

static unsigned char find(unsigned char k) {
    unsigned char i;
    for (i = 0; i < 64; i++)
        if (tab[i] == k) break;
    return i;
}

static int nested(void) {
    unsigned char i, j, cnt = 0, s = 0;
    for (i = 0; i < 20; i++)
        for (j = i + 1; j < 20; j++)
            if (((i + j) & 3) == 0) { cnt++; s += j; }
    return cnt == 45 && s == 83;
}

static unsigned char xorcopy(unsigned char m) {
    const unsigned char *p = tab;
    unsigned char i, acc = 0;
    for (i = 0; i < 64; i++) {
        unsigned char v = *p++ ^ m;
        dst[i] = v;
        acc += v;
    }
    return acc;
}

static unsigned char calls;
static void tick(void) { calls++; }

static int countdown(unsigned char n) {
    unsigned char c = 0, k;
    while (n >= 3) { n -= 3; c++; }
    for (k = 0; k < 5; k++) tick();
    return c == 66 && n == 2 && calls == 5 && k == 5;
}

static unsigned char evens(void) {
    unsigned char i, s = 0;
    for (i = 0; i < 64; i++) {
        if (tab[i] & 1) continue;
        s += tab[i];
    }
    return s;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    for (i = 0; i < 64; i++) tab[i] = (unsigned char)(i * 37 + 11);
    OUT[0] = crc8(tab, 64) == 0xA3 ? 1 : 0;
    OUT[1] = find(tab[vkey]) == 41 ? 1 : 0;
    OUT[2] = nested() ? 1 : 0;
    OUT[3] = xorcopy(vmask) == 160 && dst[10] == 39 ? 1 : 0;
    OUT[4] = countdown(vstart) ? 1 : 0;
    OUT[5] = evens() == 96 ? 1 : 0;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
