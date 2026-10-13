/* test_zp_pointers_xemu.c — pointers in the ZP register pool in leaf functions
 *
 * A leaf function (no calls, no inline asm) keeps its pointer locals in ZP
 * registers, and a pointer parameter used twice or more is copied into one
 * at the start of the body, so dereferences use ($zp),y in place.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a pointer parameter walked through an array (static allocation)
 *   [1] two pointer parameters (string copy)
 *   [2] a pointer local in a leaf function
 *   [3] a goto back to the function's first statement (the copy runs once)
 *   [4] a leaf function with a stack frame (#pragma cc45 recurse)
 *   [5] a parameter whose address is taken stays in memory
 *   [6] the same functions called through function pointers
 *   [7] post-increment/decrement (*p++, *p--, int *ip++), where the access
 *       moves before the increment, and where it must not: p read again
 *       after p++ in the same expression, a call between
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned sum(const unsigned char *p, unsigned char n) {
    unsigned s = 0;
    while (n--) s += *p++;
    return s;
}

static void copy(char *d, const char *s) { while ((*d++ = *s++)) ; }

static int count(const char *str, char c) {
    const char *s;
    int n = 0;
    for (s = str; *s; s++) if (*s == c) n++;
    return n;
}

static void fill_down(unsigned char *p, unsigned char n) {
again:
    *p++ = n;
    if (--n) goto again;
}

#pragma cc45 recurse
static unsigned char last(const unsigned char *p, unsigned char n) {
    while (--n) p++;
    return *p + p[0] - *p;
}

static unsigned char via_addr(unsigned char *p) {
    unsigned char **pp = &p;
    (*pp)++;
    return *p + p[1];
}

static unsigned char rev_sum(const unsigned char *p, unsigned char n) {
    unsigned char s = 0;
    p += n - 1;
    while (n--) s = s * 2 + *p--;          /* *p-- : dew */
    return s;
}

static unsigned isum(const unsigned *ip, unsigned char n) {
    unsigned s = 0;
    while (n--) s += *ip++;                /* stride 2 */
    return s;
}

static unsigned char pair_diff(const unsigned char *p) {
    unsigned char x = *p++;                /* then p read again */
    return (unsigned char)(*p - x);
}

static unsigned char next_after(const unsigned char *p) {
    unsigned char x = *p++ + *p;           /* (unsequenced in C: either value of p is fine) */
    return x;
}

static unsigned char twice(unsigned char c) { return (unsigned char)(c * 2); }

static unsigned char call_sum(const unsigned char *p, unsigned char n) {
    unsigned char s = 0;
    while (n--) s += twice(*p++);          /* not a leaf: p in memory */
    return s;
}

static unsigned char a[5] = {1, 2, 3, 4, 250};
static unsigned w[3] = {1000, 2000, 30000};
static char buf[16];

int main(void) {
    unsigned char b[4];
    unsigned (*fs)(const unsigned char *, unsigned char) = sum;
    int (*fc)(const char *, char) = count;

    OUT[0] = sum(a, 5) == 260 && sum(a + 1, 2) == 5;
    copy(buf, "hello");
    OUT[1] = buf[0] == 'h' && buf[4] == 'o' && buf[5] == 0;
    OUT[2] = count("banana", 'a') == 3 && count(buf, 'l') == 2 && count("", 'x') == 0;
    fill_down(b, 4);
    OUT[3] = b[0] == 4 && b[1] == 3 && b[2] == 2 && b[3] == 1;
    OUT[4] = last(a, 5) == 250 && last(a, 1) == 1;
    OUT[5] = via_addr(a) == 5;
    OUT[6] = fs(a, 3) == 6 && fc("mississippi", 's') == 4;
    {
        unsigned char nx = next_after(a + 1);
        OUT[7] = rev_sum(a, 4) == ((((4 * 2 + 3) * 2) + 2) * 2 + 1) &&
                 isum(w, 3) == 33000u && pair_diff(a + 3) == 246 &&
                 (nx == 5 || nx == 4) && call_sum(a, 4) == 20;
    }
    OUT[8] = 0xAA;
    return 0;
}
