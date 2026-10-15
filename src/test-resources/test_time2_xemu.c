/* test_time2_xemu.c — time.h: clock, CLOCKS_PER_SEC, time, gmtime,
 * localtime, mktime, asctime, ctime, difftime
 *
 * Results at $C0C0 (01 = pass):
 *   [0] clock(): frames since power-on, 100 more after 100 frames
 *   [1] CLOCKS_PER_SEC: 50 on PAL, 60 on NTSC ($D06F bit 7)
 *   [2] gmtime: the epoch (2000-01-01, a Saturday), the day's last second,
 *       before the epoch, a leap day, the ends of the time_t range
 *   [3] mktime: round trips, normalizes out-of-range fields, fills
 *       tm_wday/tm_yday
 *   [4] asctime / ctime text
 *   [5] time(): the RTC as seconds since 2000 (matches rtc_read + mktime;
 *       xemu gives the host's date, after 2020)
 *   [6] localtime == gmtime, difftime
 *   [7] $AA sentinel
 */
#include <time.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static void frames(unsigned char n) {
    while (n--) {
        while (*(volatile unsigned char *)0xD012 != 0x80) ;
        while (*(volatile unsigned char *)0xD012 == 0x80) ;
    }
}

static int is(struct tm *t, int y, int mo, int d, int h, int mi, int s, int wd, int yd) {
    return t->tm_year == y - 1900 && t->tm_mon == mo - 1 && t->tm_mday == d && t->tm_hour == h &&
           t->tm_min == mi && t->tm_sec == s && t->tm_wday == wd && t->tm_yday == yd && t->tm_isdst == 0;
}

static int t_clock(void) {
    clock_t a, b;
    frames(1);
    a = clock();
    frames(100);
    b = clock();
    return a > 0 && b - a >= 99 && b - a <= 101;
}

static int t_gmtime(void) {
    time_t t;
    int ok;
    t = 0;           ok = is(gmtime(&t), 2000, 1, 1, 0, 0, 0, 6, 0);
    t = 86399L;      ok = ok && is(gmtime(&t), 2000, 1, 1, 23, 59, 59, 6, 0);
    t = -1L;         ok = ok && is(gmtime(&t), 1999, 12, 31, 23, 59, 59, 5, 364);
    t = 5184000L;    ok = ok && is(gmtime(&t), 2000, 3, 1, 0, 0, 0, 3, 60);
    t = 762525015L;  ok = ok && is(gmtime(&t), 2024, 2, 29, 12, 30, 15, 4, 59);
    t = 2147483647L; ok = ok && is(gmtime(&t), 2068, 1, 19, 3, 14, 7, 4, 18);
    t = -2147483647L - 1; ok = ok && is(gmtime(&t), 1931, 12, 13, 20, 45, 52, 0, 346);
    return ok;
}

static int t_mktime(void) {
    struct tm tm;
    int ok;
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 124; tm.tm_mon = 1; tm.tm_mday = 29; tm.tm_hour = 12; tm.tm_min = 30; tm.tm_sec = 15;
    ok = mktime(&tm) == 762525015L && tm.tm_wday == 4 && tm.tm_yday == 59;
    /* month 12 is January of the next year; day 0 is the last of the
     * previous month; 90 minutes carry into the hour */
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 123; tm.tm_mon = 12; tm.tm_mday = 1;
    ok = ok && mktime(&tm) >= 0 && is(&tm, 2024, 1, 1, 0, 0, 0, 1, 0);
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 124; tm.tm_mon = 2; tm.tm_mday = 0; tm.tm_min = 90;
    ok = ok && mktime(&tm) > 0 && is(&tm, 2024, 2, 29, 1, 30, 0, 4, 59);
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 100; tm.tm_mon = 0; tm.tm_mday = 1; tm.tm_sec = -1;
    ok = ok && mktime(&tm) == -1L && is(&tm, 1999, 12, 31, 23, 59, 59, 5, 364);
    return ok;
}

static int t_asctime(void) {
    time_t t = 762525015L;
    struct tm tm;
    int ok = strcmp(ctime(&t), "Thu Feb 29 12:30:15 2024\n") == 0;
    t = 0;
    ok = ok && strcmp(ctime(&t), "Sat Jan  1 00:00:00 2000\n") == 0;
    memset(&tm, 0, sizeof tm);
    tm.tm_year = 99; tm.tm_mon = 11; tm.tm_mday = 31; tm.tm_hour = 23; tm.tm_min = 5; tm.tm_sec = 9;
    tm.tm_wday = 5;
    return ok && strcmp(asctime(&tm), "Fri Dec 31 23:05:09 1999\n") == 0;
}

static int t_time(void) {
    struct tm r;
    time_t a, b, t2;
    a = time(&t2);
    rtc_read(&r);
    b = mktime(&r);
    /* after 2020-01-01 (631152000) and within two seconds of the RTC */
    return a == t2 && a > 631152000L && b - a >= 0 && b - a <= 2 && gmtime(&a)->tm_year >= 120;
}

static int t_misc(void) {
    time_t t = 762525015L;
    struct tm g;
    g = *gmtime(&t);
    struct tm *l = localtime(&t);
    return l->tm_year == g.tm_year && l->tm_mday == g.tm_mday && l->tm_hour == g.tm_hour &&
           difftime(100L, 40L) == 60L && difftime(-5L, 5L) == -10L;
}

int main(void) {
    OUT[0] = t_clock();
    OUT[1] = CLOCKS_PER_SEC == ((*(volatile unsigned char *)0xD06F & 0x80) ? 60 : 50) &&
             (CLOCKS_PER_SEC == 50 || CLOCKS_PER_SEC == 60);
    OUT[2] = t_gmtime();
    OUT[3] = t_mktime();
    OUT[4] = t_asctime();
    OUT[5] = t_time();
    OUT[6] = t_misc();
    OUT[7] = 0xAA;
    return 0;
}
