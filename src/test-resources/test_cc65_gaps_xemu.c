/* test_cc65_gaps_xemu.c — cc65's string, stdlib, ctype, inttypes, signal,
 * locale, time and zlib (crc32) functions the library added
 *
 * Results at $C0C0 (01 = pass):
 *   [0] strcasecmp / strncasecmp / stricmp / strnicmp
 *   [1] strcasestr, strlwr / strupr / strlower / strupper
 *   [2] stpcpy, strndup, bzero, strcoll, strxfrm
 *   [3] strqtok
 *   [4] ultoa, isascii / toascii
 *   [5] _heapmemavail / _heapmaxavail / _heapblocksize / _heapadd,
 *       posix_memalign (the heap whole again after the frees)
 *   [6] getenv / putenv / system, _swap, _randomize
 *   [7] inttypes.h: PRI macros, imaxabs, imaxdiv, strtoimax, strtoumax
 *   [8] signal / raise
 *   [9] setlocale / localeconv
 *  [10] strftime (an ISO week of the year before too), too small a buffer
 *  [11] clock_gettime / clock_getres / clock_settime, tzset
 *  [12] crc32 ("123456789": $CBF43926; in two pieces; NULL)
 *  [13] $AA sentinel
 */
#pragma cc45 signed_default

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <locale.h>
#include <time.h>
#include <zlib.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int t_casecmp(void) {
    return strcasecmp("Hello", "hELLO") == 0 && strcasecmp("a", "B") < 0 && strcasecmp("b", "A") > 0 &&
           strcasecmp("ab", "abc") < 0 && stricmp("XyZ", "xYz") == 0 &&
           strncasecmp("abcX", "ABCy", 3) == 0 && strncasecmp("abcX", "ABCy", 4) != 0 &&
           strnicmp("Q", "q", 0) == 0 && strnicmp("abc", "ABD", 2) == 0;
}

static int t_case(void) {
    char s[8];
    const char *h = "Hello World";
    int ok = strcasestr(h, "WORLD") == h + 6 && strcasestr(h, "") == h && strcasestr(h, "worlds") == NULL &&
             strcasestr(h, "LO w") == h + 3;
    strcpy(s, "MiXeD1");
    ok = ok && strlwr(s) == s && strcmp(s, "mixed1") == 0;
    ok = ok && strupr(s) == s && strcmp(s, "MIXED1") == 0;
    ok = ok && strcmp(strlower(s), "mixed1") == 0 && strcmp(strupper(s), "MIXED1") == 0;
    return ok;
}

static int t_misc(void) {
    char buf[12], *d;
    unsigned char z[6] = { 1, 2, 3, 4, 5, 6 };
    int ok = stpcpy(buf, "abc") == buf + 3 && strcmp(buf, "abc") == 0;
    d = strndup("hello", 3);
    ok = ok && d && strcmp(d, "hel") == 0;
    free(d);
    d = strndup("hi", 10);
    ok = ok && d && strcmp(d, "hi") == 0;
    free(d);
    bzero(z, 4);
    ok = ok && z[0] == 0 && z[3] == 0 && z[4] == 5;
    ok = ok && strcoll("a", "b") < 0 && strcoll("x", "x") == 0;
    ok = ok && strxfrm(buf, "hello", 10) == 5 && strcmp(buf, "hello") == 0;
    memset(buf, 'x', 4);
    ok = ok && strxfrm(buf, "hello", 3) == 5 && buf[0] == 'x';
    return ok;
}

static int t_qtok(void) {
    char s[] = "a \"b c\" d\"e f\"";
    char *t1 = strqtok(s, " ");
    char *t2 = strqtok(NULL, " ");
    char *t3 = strqtok(NULL, " ");
    char *t4 = strqtok(NULL, " ");
    char *t5 = strqtok(NULL, " ");
    return t1 && !strcmp(t1, "a") && t2 && !strcmp(t2, "b c") && t3 && !strcmp(t3, "d") && t4 &&
           !strcmp(t4, "e f") && t5 == NULL;
}

static int t_conv(void) {
    char a[12], b[12];
    int ok = strcmp(ultoa(4000000000UL, a, 10), "4000000000") == 0 && strcmp(ltoa(-5L, b, 10), "-5") == 0;
    ultoa(0xFFFFFFFFUL, a, 16);
    ltoa(-1L, b, 16);
    ok = ok && strcmp(a, b) == 0 && strlen(a) == 8;
    return ok && isascii(0x41) && !isascii(0xC1) && !isascii(-1) && toascii(0xC1) == 0x41;
}

static unsigned char extra[200];

static int t_heap(void) {
    unsigned int before, after, i;
    void *p, *q;
    char *r;
    int ok;
    p = malloc(11);
    ok = p && _heapblocksize(p) == 12;
    free(p);
    before = _heapmemavail();
    ok = ok && before > 0 && _heapmaxavail() <= before;
    /* aligned blocks, then the heap whole again */
    p = q = NULL;
    ok = ok && posix_memalign(&p, 64, 100) == 0 && ((unsigned int)p & 63) == 0;
    ok = ok && posix_memalign(&q, 256, 10) == 0 && ((unsigned int)q & 255) == 0;
    for (i = 0; i < 100; i++) ((char *)p)[i] = (char)i;
    ok = ok && posix_memalign(&p, 3, 10) == EINVAL;
    free(q);
    free(p);
    ok = ok && _heapmemavail() == before;
    /* memory given to the heap: malloc takes it first (lower addresses) */
    _heapadd(extra, sizeof extra);
    after = _heapmemavail();
    ok = ok && (after == before + 198 || after == before + 200);
    r = (char *)malloc(150);
    ok = ok && r >= (char *)extra && r < (char *)extra + sizeof extra;
    free(r);
    return ok;
}

static char e1[] = "HOME=/x", e2[] = "HOME=/y", e3[] = "A=1", e4[] = "HOME";

static int t_env(void) {
    unsigned char s1[3] = { 1, 2, 3 }, s2[3] = { 7, 8, 9 };
    int ok = getenv("HOME") == NULL && putenv(e1) == 0 && strcmp(getenv("HOME"), "/x") == 0;
    ok = ok && putenv(e2) == 0 && strcmp(getenv("HOME"), "/y") == 0 && putenv(e3) == 0 &&
         strcmp(getenv("A"), "1") == 0;
    ok = ok && putenv(e4) == 0 && getenv("HOME") == NULL && strcmp(getenv("A"), "1") == 0;
    ok = ok && system(NULL) == 0 && system("dir") == -1;
    _swap(s1, s2, 3);
    ok = ok && s1[0] == 7 && s1[2] == 9 && s2[0] == 1 && s2[2] == 3;
    _randomize();
    return ok && rand() <= RAND_MAX;
}

static int t_inttypes(void) {
    char b[16];
    char *end;
    imaxdiv_t d = imaxdiv(17, 5);
    int ok;
    sprintf(b, "%" PRId32, (int32_t)-123456);
    ok = strcmp(b, "-123456") == 0;
    sprintf(b, "%" PRIu16 "/%" PRIX8, (uint16_t)65535, (uint8_t)0xAB);
    ok = ok && strcmp(b, "65535/AB") == 0;
    ok = ok && imaxabs(-5) == 5 && d.quot == 3 && d.rem == 2;
    ok = ok && strtoimax("-99z", &end, 10) == -99 && *end == 'z' && strtoumax("ff", NULL, 16) == 255;
    return ok;
}

static volatile int got;
static void on_sig(int s) {
    got = s + 1;
}

static int t_signal(void) {
    int ok = signal(SIGINT, on_sig) == SIG_DFL && raise(SIGINT) == 0 && got == SIGINT + 1;
    /* the handler went back to SIG_DFL when it was called */
    ok = ok && signal(SIGINT, SIG_IGN) == SIG_DFL && raise(SIGINT) == 0;
    got = 0;
    ok = ok && signal(SIGTERM, on_sig) == SIG_DFL && raise(SIGTERM) == 0 && got == SIGTERM + 1;
    errno = 0;
    ok = ok && signal(99, on_sig) == SIG_ERR && errno == EINVAL && raise(99) != 0;
    return ok;
}

static int t_locale(void) {
    struct lconv *lc = localeconv();
    return strcmp(setlocale(LC_ALL, NULL), "C") == 0 && setlocale(LC_NUMERIC, "") != NULL &&
           setlocale(LC_ALL, "de_DE") == NULL && setlocale(99, "C") == NULL &&
           strcmp(lc->decimal_point, ".") == 0 && (unsigned char)lc->frac_digits == 255;
}

static int t_strftime(void) {
    struct tm tm;
    char b[72];
    int ok;
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 126;
    tm.tm_mon = 9;
    tm.tm_mday = 29;
    tm.tm_hour = 14;
    tm.tm_min = 5;
    tm.tm_sec = 9;
    mktime(&tm);
    ok = strftime(b, sizeof b, "%Y-%m-%d %H:%M:%S", &tm) == 19 && strcmp(b, "2026-10-29 14:05:09") == 0;
    strftime(b, sizeof b, "%a %A %b %B", &tm);
    ok = ok && strcmp(b, "Thu Thursday Oct October") == 0;
    strftime(b, sizeof b, "%j %I %p %u %w %U %W %V %G %g", &tm);
    ok = ok && strcmp(b, "302 02 PM 4 4 43 43 44 2026 26") == 0;
    strftime(b, sizeof b, "%c|%D|%F|%T|%R|%%|%C", &tm);
    ok = ok && strcmp(b, "Thu Oct 29 14:05:09 2026|10/29/26|2026-10-29|14:05:09|14:05|%|20") == 0;
    /* too small: 0 */
    ok = ok && strftime(b, 5, "%Y-%m", &tm) == 0;
    /* 2027-01-01 (a Friday) is in ISO week 53 of 2026; %e pads with a space */
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 127;
    tm.tm_mday = 1;
    mktime(&tm);
    strftime(b, sizeof b, "%G-W%V|%e|%z|%Z.", &tm);
    return ok && strcmp(b, "2026-W53| 1|+0000|.") == 0;
}

static int t_clock(void) {
    struct timespec a, b, r;
    int ok, i;
    ok = clock_gettime(CLOCK_REALTIME, &a) == 0 && a.tv_nsec == 0 && time(NULL) - a.tv_sec <= 1;
    ok = ok && clock_gettime(CLOCK_MONOTONIC, &a) == 0 && a.tv_nsec >= 0 && a.tv_nsec < 1000000000L;
    for (i = 0; i < 30000; i++) {
        clock_gettime(CLOCK_MONOTONIC, &b);
        if (b.tv_sec != a.tv_sec || b.tv_nsec != a.tv_nsec) break;
    }
    ok = ok && (b.tv_sec > a.tv_sec || (b.tv_sec == a.tv_sec && b.tv_nsec > a.tv_nsec));
    ok = ok && clock_getres(CLOCK_MONOTONIC, &r) == 0 && r.tv_sec == 0 &&
         (r.tv_nsec == 20000000L || r.tv_nsec == 16666666L);
    ok = ok && clock_getres(CLOCK_REALTIME, &r) == 0 && r.tv_sec == 1;
    errno = 0;
    ok = ok && clock_gettime(7, &a) == -1 && errno == EINVAL && clock_settime(CLOCK_MONOTONIC, &a) == -1;
    a.tv_sec = time(NULL);
    a.tv_nsec = 0;
    ok = ok && clock_settime(CLOCK_REALTIME, &a) == 0;
    tzset();
    return ok;
}

static int t_crc(void) {
    const unsigned char *s = (const unsigned char *)"123456789";
    unsigned long c = crc32(0, NULL, 0);
    unsigned long whole = crc32(c, s, 9);
    unsigned long part = crc32(crc32(0, s, 4), s + 4, 5);
    return c == 0 && whole == 0xCBF43926UL && part == whole && crc32(0, s, 0) == 0;
}

int main(void) {
    OUT[0] = t_casecmp();
    OUT[1] = t_case();
    OUT[2] = t_misc();
    OUT[3] = t_qtok();
    OUT[4] = t_conv();
    OUT[5] = t_heap();
    OUT[6] = t_env();
    OUT[7] = t_inttypes();
    OUT[8] = t_signal();
    OUT[9] = t_locale();
    OUT[10] = t_strftime();
    OUT[11] = t_clock();
    OUT[12] = t_crc();
    OUT[13] = 0xAA;
    return 0;
}
