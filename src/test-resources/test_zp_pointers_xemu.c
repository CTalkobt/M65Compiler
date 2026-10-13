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
 *   [7] $AA sentinel
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

static unsigned char a[5] = {1, 2, 3, 4, 250};
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
    OUT[7] = 0xAA;
    return 0;
}
