/* mktime_minimal.c — Minimal mktime implementation without bloat
 *
 * This is a stripped-down version of rtc.c that only implements mktime(),
 * avoiding static array initialization issues that might cause crashes.
 */

#include <time.h>

/* Days in each month (non-leap year) - stack-allocated to avoid static issues */
static int is_leap_year(int year) {
    if (year % 4 != 0) return 0;
    if (year % 100 != 0) return 1;
    if (year % 400 == 0) return 1;
    return 0;
}

static int day_of_year(int year, int mon, int mday) {
    static int days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int doy = 0;
    int i;
    for (i = 0; i < mon; i++) {
        doy += days_in_month[i];
        if (i == 1 && is_leap_year(year)) doy++;
    }
    return doy + mday - 1;
}

/* Convert struct tm to time_t (seconds since 2000-01-01 00:00:00) */
time_t mktime(struct tm *tm) {
    long days = 0;
    int y;
    int full_year = tm->tm_year + 1900;

    /* Count days from 2000 to tm_year */
    for (y = 2000; y < full_year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    days += day_of_year(full_year, tm->tm_mon, tm->tm_mday);

    /* Fill in computed fields */
    tm->tm_yday = day_of_year(full_year, tm->tm_mon, tm->tm_mday);

    return days * 86400L + tm->tm_hour * 3600L + tm->tm_min * 60L + tm->tm_sec;
}
