/* test_stdlib2_xemu.c — stdlib/string/ctype functions not covered elsewhere
 *
 * Results at $C0C0 (01 = pass):
 *   [0] strtol: signs, bases 0/8/10/16/36, endptr, uppercase hex, overflow
 *   [1] strtoul: ULONG_MAX, overflow (ERANGE), '-' negates, endptr
 *   [2] atol, atof
 *   [3] div, ldiv (truncation toward zero, signs of the remainder)
 *   [4] srand/rand: a seed repeats its sequence, results within RAND_MAX
 *   [5] utoa, ltoa (bases 2, 10, 16, 36; negative; LONG_MIN)
 *   [6] strtok (several delimiters, empty tokens skipped, NULL continues),
 *       strtok_r (two strings at once)
 *   [7] ctype: ispunct; 'A' (cc45 writes $C1) is an upper-case letter
 *   [8] atexit: handlers run in reverse order when main returns (or, built
 *       with -DVIA_EXIT, when exit() is called)
 *   [9] $AA sentinel (written by the last handler)
 *
 * Built with -DVIA_ASSERT or -DVIA_ASSERT_MSG, main ends with a failing
 * assert / assert_msg: the program stops with exit status $FFFE (at $FD/$FE)
 * and the atexit handlers do not run.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int t_strtol(void) {
    char *end;
    char s1[] = "  -123xyz";
    int ok = strtol(s1, &end, 10) == -123L && end == s1 + 6;
    ok = ok && strtol("0x1F", 0, 16) == 31 && strtol("0X1f", 0, 0) == 31 && strtol("1F", 0, 16) == 31;
    ok = ok && strtol("017", 0, 0) == 15 && strtol("017", 0, 10) == 17 && strtol("777", 0, 8) == 511;
    ok = ok && strtol("z", 0, 36) == 35 && strtol("Z", 0, 36) == 35 && strtol("+42", 0, 10) == 42;
    ok = ok && strtol("2147483647", 0, 10) == LONG_MAX && strtol("-2147483648", 0, 10) == LONG_MIN;
    {
        char s2[] = "abc";
        ok = ok && strtol(s2, &end, 10) == 0 && end == s2;
        ok = ok && strtol("--5", &end, 10) == 0;
    }
    errno = 0;
    ok = ok && strtol("99999999999", 0, 10) == LONG_MAX && errno == ERANGE;
    errno = 0;
    ok = ok && strtol("-99999999999", 0, 10) == LONG_MIN && errno == ERANGE;
    errno = 0;
    ok = ok && strtol("2147483648", 0, 10) == LONG_MAX && errno == ERANGE;
    errno = 0;
    ok = ok && strtol("12", 0, 10) == 12 && errno == 0;
    {
        char s3[] = "0x";   /* no hex digit: the 0 is the number */
        ok = ok && strtol(s3, &end, 0) == 0 && end == s3 + 1;
    }
    return ok;
}

static int t_strtoul(void) {
    char *end;
    char s1[] = "4294967295 rest";
    int ok = strtoul(s1, &end, 10) == ULONG_MAX && *end == ' ';
    ok = ok && strtoul("ff", 0, 16) == 255 && strtoul("FFFF", 0, 16) == 65535UL;
    ok = ok && strtoul("deadBEEF", 0, 16) == 0xDEADBEEFUL && strtoul("1010", 0, 2) == 10;
    errno = 0;
    ok = ok && strtoul("4294967296", 0, 10) == ULONG_MAX && errno == ERANGE;
    ok = ok && strtoul("-1", 0, 10) == ULONG_MAX && strtoul("-2", 0, 10) == ULONG_MAX - 1;
    {
        char s2[] = "  xyz";
        ok = ok && strtoul(s2, &end, 0) == 0 && end == s2;
    }
    return ok;
}

static int t_atol_atof(void) {
    float f = atof("3.5");
    float g = atof("-0.25");
    float h = atof("1e3");
    char *end;
    char s1[] = "1e", s2[] = ".", s3[] = "x", s4[] = "2.5e+2z";
    int ok = strtof(s1, &end) == 1.0 && end == s1 + 1 && strtof(s2, &end) == 0.0 && end == s2 &&
             strtof(s3, &end) == 0.0 && end == s3 && strtof(s4, &end) == 250.0 && *end == 'z' &&
             atof(".5") == 0.5 && atof("0.125") == 0.125 && atof("1.25E2") == 125.0 &&
             atof("1000000000000") == 1000000000000.0 && atof("-7") == -7.0;
    return ok && atol("-70000") == -70000L && atol("  +12") == 12 && atol("123456789") == 123456789L &&
           f == 3.5 && g == -0.25 && h == 1000.0;
}

static int t_div(void) {
    div_t d1 = div(-7, 2);
    div_t d2 = div(7, -2);
    div_t d3 = div(20, 5);
    ldiv_t l1 = ldiv(-100000L, 7L);
    ldiv_t l2 = ldiv(100000L, -7L);
    ldiv_t l3 = ldiv(1234567L, 1000L);
    return d1.quot == -3 && d1.rem == -1 && d2.quot == -3 && d2.rem == 1 && d3.quot == 4 && d3.rem == 0 &&
           l1.quot == -14285L && l1.rem == -5 && l2.quot == -14285L && l2.rem == 5 &&
           l3.quot == 1234 && l3.rem == 567;
}

static int t_rand(void) {
    int a[5], i, ok = 1;
    srand(1);
    for (i = 0; i < 5; i++) a[i] = rand();
    srand(1);
    for (i = 0; i < 5; i++) if (rand() != a[i]) ok = 0;
    for (i = 0; i < 5; i++) if (a[i] < 0 || a[i] > RAND_MAX) ok = 0;
    /* C's reference generator: seed 1 gives 16838, 5758, 10113, 17515, 31051 */
    ok = ok && a[0] == 16838 && a[1] == 5758 && a[2] == 10113 && a[3] == 17515 && a[4] == 31051;
    srand(2);
    ok = ok && rand() != a[0];
    return ok;
}

static int t_toa(void) {
    char b[40];
    int ok = strcmp(utoa(65535u, b, 10), "65535") == 0;
    ok = ok && strcmp(utoa(0xBEEFu, b, 16), "beef") == 0;
    ok = ok && strcmp(utoa(5, b, 2), "101") == 0 && strcmp(utoa(0, b, 10), "0") == 0;
    ok = ok && strcmp(ltoa(-123456L, b, 10), "-123456") == 0;
    ok = ok && strcmp(ltoa(255L, b, 2), "11111111") == 0;
    ok = ok && strcmp(ltoa(LONG_MAX, b, 10), "2147483647") == 0;
    ok = ok && strcmp(ltoa(LONG_MIN, b, 10), "-2147483648") == 0;
    ok = ok && strcmp(ltoa(35L, b, 36), "z") == 0 && strcmp(ltoa(0x12345678L, b, 16), "12345678") == 0;
    return ok;
}

static int t_strtok(void) {
    char s[] = ",,a,b;;c,";
    char *t1 = strtok(s, ",;");
    char *t2 = strtok(0, ",;");
    char *t3 = strtok(0, ",;");
    char *t4 = strtok(0, ",;");
    char *t5 = strtok(0, ",;");
    char e[] = ";;";
    /* strtok_r: two strings at once */
    char p[] = "x y", q[] = "1-2-3";
    char *sp, *sq;
    char *p1 = strtok_r(p, " ", &sp), *q1 = strtok_r(q, "-", &sq), *p2 = strtok_r(0, " ", &sp);
    char *q2 = strtok_r(0, "-", &sq), *q3 = strtok_r(0, "-", &sq), *q4 = strtok_r(0, "-", &sq);
    return t1 && strcmp(t1, "a") == 0 && t2 && strcmp(t2, "b") == 0 && t3 && strcmp(t3, "c") == 0 &&
           t4 == 0 && t5 == 0 && strtok(e, ";") == 0 &&
           p1 == p && q1 == q && p2 == p + 2 && q2 == q + 2 && q3 == q + 4 && q4 == 0 && sq == 0 &&
           strtok_r(0, " ", &sp) == 0;
}

static int t_ispunct(void) {
    return ispunct(0x21) && ispunct(0x7E) && ispunct(0x2E) && ispunct(0x40) &&
           !ispunct(0x20) && !ispunct(0x30) && !ispunct(0x41) && !ispunct(0x61) && !ispunct(0x0D) &&
           !ispunct('A') && isupper('A') && isupper('Z') && !islower('A') && islower('a') &&
           isalpha('Q') && isalnum('Q') && isxdigit('F') && !isxdigit('G') && isupper(0xC1) &&
           tolower('A') == 'a' && tolower(0xC1) == 'a' && toupper('a') == 0xC1 && tolower('5') == '5';
}

static unsigned char order;

static void h_first(void) {
    /* registered first, so it runs last */
    OUT[8] = order == 1;
    OUT[9] = 0xAA;
}

static void h_second(void) {
    order = 1;
}

static void finish(void) {
#ifdef VIA_EXIT
    exit(0);
#endif
#ifdef VIA_ASSERT
    assert(OUT[0] == 0x55);
#endif
#ifdef VIA_ASSERT_MSG
    assert_msg(OUT[0] == 0x55, "expected failure");
#endif
}

int main(void) {
    OUT[0] = t_strtol();
    OUT[1] = t_strtoul();
    OUT[2] = t_atol_atof();
    OUT[3] = t_div();
    OUT[4] = t_rand();
    OUT[5] = t_toa();
    OUT[6] = t_strtok();
    OUT[7] = t_ispunct();
    OUT[8] = 0;
    if (atexit(h_first) != 0 || atexit(h_second) != 0) return 1;
    finish();
    return 0;
}
