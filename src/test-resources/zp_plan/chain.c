/* zp_plan/chain.c — a call chain whose functions keep pointers across calls
 * (their base-page blocks are active together) */
unsigned char tick(unsigned char v) { volatile unsigned char t = v; return t; }

static unsigned int step(const unsigned char *p, unsigned char n) {
    unsigned int s = 0;
    while (n--) s += tick(*p++);
    return s;
}

static unsigned int walk(const unsigned char *a, const unsigned char *b, unsigned char n) {
    const unsigned char *x = a, *y = b;
    unsigned int s = step(x, n);
    s += step(y, n);
    return s + step(x, n) + tick(*x);           /* x and y kept across the calls */
}

unsigned int chain(const unsigned char *a, const unsigned char *b, unsigned char n) {
    const unsigned char *pa = a, *pb = b;
    unsigned int s = walk(pa, pb, n);
    return s + step(pb, n) - tick(*pa);
}
