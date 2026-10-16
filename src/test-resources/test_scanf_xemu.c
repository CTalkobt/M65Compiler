/* test_scanf_xemu.c — the scanf engine (sscanf): conversions, widths, sets,
 * sizes, suppression and the counts returned
 *
 * Results at $C0C0 (01 = pass):
 *   [0] integers: %d %i (0x, 0, decimal) %o %x ($ and 0x prefixes, either
 *       case) %u %p, signs
 *   [1] sizes: %hhd (one byte), %hd, %ld, %lld (8 bytes, the sign above),
 *       %n with them
 *   [2] widths: %3d%2d, %2s, %4c (no terminator, white space kept), %c not
 *       skipping white space
 *   [3] sets: %[a-z], %[^,], %[]x] (']' first), ranges and '-' last, width
 *   [4] %*d (suppressed, not counted), %%, literals, %n, white space in the
 *       format matching any amount (or none)
 *   [5] the count returned: EOF for empty input, 0 at a matching failure,
 *       the items before a mismatch; upper-case conversions (%X %D)
 *   [6] floats (the float scanf is linked): %f %e %g %lf %E, a width, %*f
 *       (not counted), signs and exponents, a matching failure
 *   [7] $AA sentinel
 */
#include <stdio.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int t_ints(void) {
    int d, i1, i2, i3, o, x1, x2, x3;
    unsigned int u;
    void *p;
    int ok = sscanf("-12 +7", "%d %d", &d, &i1) == 2 && d == -12 && i1 == 7;
    ok = ok && sscanf("0x1F 017 42", "%i %i %i", &i1, &i2, &i3) == 3 && i1 == 31 && i2 == 15 && i3 == 42;
    ok = ok && sscanf("-0x10", "%i", &i1) == 1 && i1 == -16;
    ok = ok && sscanf("17 777", "%o %o", &o, &i1) == 2 && o == 15 && i1 == 511;
    ok = ok && sscanf("$ff 0XAb 7f", "%x %x %x", &x1, &x2, &x3) == 3 && x1 == 255 && x2 == 0xAB && x3 == 0x7F;
    ok = ok && sscanf("65535", "%u", &u) == 1 && u == 65535u;
    ok = ok && sscanf("c000", "%p", &p) == 1 && p == (void *)0xC000;
    return ok;
}

static int t_sizes(void) {
    unsigned char b[3] = { 0x11, 0x22, 0x33 };
    long l;
    unsigned char ll[9];
    int h, n, n2, i, ok;
    ok = sscanf("200", "%hhd", &b[1]) == 1 && b[0] == 0x11 && b[1] == 200 && b[2] == 0x33;
    ok = ok && sscanf("-300", "%hd", &h) == 1 && h == -300;
    ok = ok && sscanf("100000 x", "%ld%n", &l, &n) == 1 && l == 100000L && n == 6;
    for (i = 0; i < 9; i++) ll[i] = 0x77;
    ok = ok && sscanf("-5", "%lld", ll) == 1 && ll[0] == 0xFB && ll[3] == 0xFF && ll[7] == 0xFF && ll[8] == 0x77;
    for (i = 0; i < 9; i++) ll[i] = 0x77;
    ok = ok && sscanf("70000", "%lld", ll) == 1 && ll[0] == 0x70 && ll[1] == 0x11 && ll[2] == 1 && ll[4] == 0 &&
         ll[7] == 0 && ll[8] == 0x77;
    ok = ok && sscanf("ab", "%*c%hhn%n", &b[0], &n2) == 0 && b[0] == 1 && n2 == 1;
    return ok;
}

static int t_widths(void) {
    int a, b;
    char s[8], c[6];
    int ok = sscanf("12345", "%3d%2d", &a, &b) == 2 && a == 123 && b == 45;
    ok = ok && sscanf("hello", "%2s", s) == 1 && strcmp(s, "he") == 0;
    memset(c, '#', sizeof c);
    ok = ok && sscanf(" a b", "%4c", c) == 1 && c[0] == ' ' && c[1] == 'a' && c[3] == 'b' && c[4] == '#';
    ok = ok && sscanf("  q", "%c", c) == 1 && c[0] == ' ';
    ok = ok && sscanf("  q", " %c", c) == 1 && c[0] == 'q';
    return ok;
}

static int t_sets(void) {
    char a[16], b[16], c[16];
    int n, ok;
    ok = sscanf("abc123", "%[a-z]%s", a, b) == 2 && strcmp(a, "abc") == 0 && strcmp(b, "123") == 0;
    ok = ok && sscanf("one two,three", "%[^,],%s", a, b) == 2 && strcmp(a, "one two") == 0 && strcmp(b, "three") == 0;
    ok = ok && sscanf("]x]y", "%[]x]%s", a, b) == 2 && strcmp(a, "]x]") == 0 && strcmp(b, "y") == 0;
    ok = ok && sscanf("a-b-c.d", "%[a-c-]%n", a, &n) == 1 && strcmp(a, "a-b-c") == 0 && n == 5;
    ok = ok && sscanf("0123456789", "%4[0-9]%[0-9]", a, c) == 2 && strcmp(a, "0123") == 0 && strcmp(c, "456789") == 0;
    ok = ok && sscanf("xyz", "%[0-9]", a) == 0;
    return ok;
}

static int t_misc(void) {
    int a, b, n;
    int ok = sscanf("1 2", "%*d %d", &a) == 1 && a == 2;
    ok = ok && sscanf("5%6", "%d%%%d", &a, &b) == 2 && a == 5 && b == 6;
    ok = ok && sscanf("5 % 6", "%d %% %d", &a, &b) == 2 && a == 5 && b == 6;
    ok = ok && sscanf("x=10, y=20", "x=%d, y=%d%n", &a, &b, &n) == 2 && a == 10 && b == 20 && n == 10;
    ok = ok && sscanf("7,8", "%d , %d", &a, &b) == 2 && a == 7 && b == 8;
    ok = ok && sscanf("  \t\r9", " %d", &a) == 1 && a == 9;
    return ok;
}

static int t_counts(void) {
    int a = 0, b = 0, ok;
    char w[8];
    ok = sscanf("", "%d", &a) == EOF && sscanf("   ", "%d", &a) == EOF;
    ok = ok && sscanf("abc", "%d", &a) == 0;
    ok = ok && sscanf("1 x", "%d %d", &a, &b) == 1 && a == 1;
    ok = ok && sscanf("1;2", "%d,%d", &a, &b) == 1;
    ok = ok && sscanf("3", "%d %d", &a, &b) == 1 && a == 3;
    ok = ok && sscanf("ff 12 hi", "%X %D %S", &a, &b, w) == 3 && a == 255 && b == 12 && strcmp(w, "hi") == 0;
    ok = ok && sscanf("", "") == 0 && sscanf("x", "%s%d", w, &a) == 1;
    return ok;
}

static int t_floats(void) {
    float a, b, c, d;
    int n, ok;
    ok = sscanf("1.5 -2.25e1 3e-2", "%f %e %g", &a, &b, &c) == 3 && a == 1.5 && b == -22.5 && c > 0.0299 &&
         c < 0.0301;
    ok = ok && sscanf("12345.5", "%3f%f", &a, &b) == 2 && a == 123.0 && b == 45.5;
    ok = ok && sscanf("7.5 8.25", "%*f %lf%n", &d, &n) == 1 && d == 8.25 && n == 8;
    ok = ok && sscanf("+.5E+1", "%E", &a) == 1 && a == 5.0;
    ok = ok && sscanf("x", "%f", &a) == 0 && sscanf("-", "%f", &a) == 0;
    return ok;
}

int main(void) {
    OUT[0] = t_ints();
    OUT[1] = t_sizes();
    OUT[2] = t_widths();
    OUT[3] = t_sets();
    OUT[4] = t_misc();
    OUT[5] = t_counts();
    OUT[6] = t_floats();
    OUT[7] = 0xAA;
    return 0;
}
