/* time.h — Time functions for cc45 / MEGA65
 *
 * Jiffy clock: clock() and time() use the 60Hz KERNAL timer for elapsed time.
 * Calendar time: struct tm and related functions read the MEGA65 RTC (I2C).
 *
 * The RTC is at $FFD71xx (I2C registers, BCD-encoded).
 * On real MEGA65 hardware, provides year/month/day/hour/minute/second.
 * On emulators without RTC, rtc_read() returns zeros.
 */

#pragma once

#define CLOCKS_PER_SEC 60

typedef long time_t;
typedef int clock_t;

/* Jiffy clock */
clock_t clock(void);
time_t time(time_t *tp);
long difftime(time_t t2, time_t t1);

/* Calendar time (MEGA65 RTC) */
struct tm {
    int tm_sec;     /* seconds (0-59) */
    int tm_min;     /* minutes (0-59) */
    int tm_hour;    /* hours (0-23) */
    int tm_mday;    /* day of month (1-31) */
    int tm_mon;     /* month (0-11) */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* day of week (0=Sunday) */
    int tm_yday;    /* day of year (0-365) */
    int tm_isdst;   /* DST flag (always 0) */
};

struct tm *localtime(time_t *timer);
struct tm *gmtime(time_t *timer);
time_t mktime(struct tm *tm);
char *asctime(struct tm *tm);
char *ctime(time_t *timer);

/* MEGA65-specific: read RTC directly into struct tm */
void rtc_read(struct tm *tm);
