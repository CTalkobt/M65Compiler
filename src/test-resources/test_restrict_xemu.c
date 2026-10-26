/* test_restrict_xemu.c — restrict pointer parameters (#77): loads through
 * one are reused across stores into globals and through other restrict
 * pointers (CSE) and hoisted out of loops (LICM); every other case keeps
 * reloading. Built at -O2 (and -O0 by test_xemu.sh: the same results)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] s[0], s[1] reused after stores through d and into a global
 *   [1] a loop: *k and k[1] hoisted, out[i] = in[i] * *k + k[1]
 *   [2] the same function without restrict, called with d == s (aliasing):
 *       every load after a store sees it
 *   [3] a restrict parameter the function assigns (p++): loads as written
 *   [4] int a[restrict] (an array parameter) parses and works
 *   [5] a call in the loop that changes *k through a pointer made from k:
 *       not hoisted, each iteration sees the new value
 *   [6] volatile int *restrict: every read is done
 *   [7] restrict parameters that are stepped (d++, s++): still their
 *       objects; a copy loop with stores into a global between
 *   [8] a stepped restrict pointer sees what was stored through it (p[0],
 *       p++, p[-1], p--): no load reused across those
 *   [9] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char g;
static unsigned char buf[8], src[8];
static signed int outv[6], inv[6], kv[2];

static void mix(unsigned char *restrict d, const unsigned char *restrict s) {
    d[0] = s[0] + s[1];
    g = 1;
    d[1] = s[0] - s[1];
    d[2] = s[0];
}

static void mix_alias(unsigned char *d, const unsigned char *s) {
    d[0] = s[0] + s[1];
    g = 1;
    d[1] = s[0] - s[1];
    d[2] = s[0];
}

static void scale(signed int *restrict out, const signed int *restrict in, const signed int *restrict k, unsigned char n) {
    unsigned char i;
    for (i = 0; i < n; i++) out[i] = in[i] * *k + k[1];
}

static unsigned int walk(const unsigned char *restrict p, unsigned char n) {
    unsigned int sum = 0;
    while (n--) { sum = sum * 2 + *p; p++; }
    return sum;
}

static unsigned char sum_arr(const unsigned char a[restrict], unsigned char n) {
    unsigned char s = 0, i;
    for (i = 0; i < n; i++) s += a[i];
    return s;
}

static void bump(signed int *k) { *k += 1; }

__attribute__((noinline)) static void copy2(char *restrict d, const char *restrict s) {
    while (*s) { *d = *s; g++; d[1] = *s; d++; s++; }
    *d = 0;
}

__attribute__((noinline)) static signed int step(signed int *restrict p) {
    p[0] = 1;
    p++;
    *p = p[-1] + 1;
    p--;
    return *p + p[1] * 10;
}

static signed int count_up(signed int *restrict k, unsigned char n) {
    signed int sum = 0;
    unsigned char i;
    for (i = 0; i < n; i++) { sum += *k; bump(k); }
    return sum;
}

static unsigned char poll(volatile unsigned char *restrict reg, unsigned char n) {
    unsigned char seen = 0;
    while (n--) seen += *reg;   /* the raster line: read each time */
    return seen;
}

int main(void) {
    unsigned char i, ok;

    src[0] = 10; src[1] = 3;
    mix(buf, src);
    OUT[0] = buf[0] == 13 && buf[1] == 7 && buf[2] == 10 && g == 1;

    for (i = 0; i < 6; i++) inv[i] = (signed int)i - 2;
    kv[0] = 3; kv[1] = -1;
    scale(outv, inv, kv, 6);
    ok = 1;
    for (i = 0; i < 6; i++) if (outv[i] != ((signed int)i - 2) * 3 - 1) ok = 0;
    OUT[1] = ok;

    buf[0] = 10; buf[1] = 3;
    mix_alias(buf, buf);          /* d[0] = 13, then s[0] is 13: d[1] = 10, d[2] = 13 */
    OUT[2] = buf[0] == 13 && buf[1] == 10 && buf[2] == 13;

    src[0] = 1; src[1] = 0; src[2] = 1; src[3] = 1;
    OUT[3] = walk(src, 4) == 11;

    src[0] = 5; src[1] = 6; src[2] = 7;
    OUT[4] = sum_arr(src, 3) == 18;

    kv[0] = 1;
    OUT[5] = count_up(kv, 4) == 1 + 2 + 3 + 4 && kv[0] == 5;

    {
        /* a byte the test changes between reads can't be arranged here: a
         * volatile read of the raster line, 200 times, sums more than 200
         * reads of one line would unless the line moved (it does) */
        unsigned char a = poll((volatile unsigned char *)0xD012, 1), same = 1;
        for (i = 0; i < 20 && same; i++) if (poll((volatile unsigned char *)0xD012, 1) != a) same = 0;
        OUT[6] = !same;
    }
    {
        static char to[8];
        static signed int two[2];
        g = 0;
        copy2(to, "ABC");
        OUT[7] = to[0] == 'A' && to[1] == 'B' && to[2] == 'C' && to[3] == 0 && g == 3;
        OUT[8] = step(two) == 1 + 2 * 10 && two[0] == 1 && two[1] == 2;
    }
    OUT[9] = 0xAA;
    return 0;
}
