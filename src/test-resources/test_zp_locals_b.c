/* test_zp_locals_b.c — the second module of test_zp_locals_xemu.c: functions
 * with pointer variables in their base-page blocks, called from the other
 * module (the blocks of caller and callee must not overlap) */
#define NOINLINE __attribute__((noinline))

NOINLINE static unsigned char is_c(unsigned char x, unsigned char c) { return x == c; }

/* the first c in p (a pointer kept across calls) */
unsigned char *b_find(unsigned char *p, unsigned char c) {
    while (*p && !is_c(*p, c)) p++;
    return p;
}

NOINLINE static unsigned char up(unsigned char x) { return (unsigned char)(x + 1); }

/* two pointers kept across calls: d[i] = s[i] + 1 */
void b_upper(unsigned char *d, const unsigned char *s) {
    while (*s) *d++ = up(*s++);
    *d = 0;
}
