/* time.h — Time functions for cc45 / MEGA65
 *
 * clock(): video frames since power-on (the KERNAL's frame counter);
 * CLOCKS_PER_SEC is the frame rate, 50 (PAL) or 60 (NTSC).
 * Calendar time: time_t is seconds since 2000-01-01 00:00:00. time() reads
 * the MEGA65 RTC ($FFD7110, by DMA; xemu gives the host's time); gmtime,
 * localtime (the same: the RTC has no time zone), mktime, asctime and ctime
 * convert.
 */

#pragma once

#ifndef NULL
#define NULL 0
#endif

typedef signed long time_t;
typedef unsigned long clock_t;

/* The frame rate: $D06F bit 7 is set in NTSC mode */
static inline clock_t __clocks_per_sec(void) {
    return (*(volatile unsigned char *)0xD06F & 0x80) ? 60 : 50;
}
#define CLOCKS_PER_SEC (__clocks_per_sec())

/* Frames since power-on (wraps after 2^24, about 3.9 days at 50 Hz) */
__fastcall__ clock_t clock(void);
time_t time(time_t *tp);
signed long difftime(time_t t2, time_t t1);

/* Calendar time (MEGA65 RTC) */
struct tm {
    signed int tm_sec;     /* seconds (0-59) */
    signed int tm_min;     /* minutes (0-59) */
    signed int tm_hour;    /* hours (0-23) */
    signed int tm_mday;    /* day of month (1-31) */
    signed int tm_mon;     /* month (0-11) */
    signed int tm_year;    /* years since 1900 */
    signed int tm_wday;    /* day of week (0=Sunday) */
    signed int tm_yday;    /* day of year (0-365) */
    signed int tm_isdst;   /* DST flag (always 0) */
};

struct tm *localtime(time_t *timer);
struct tm *gmtime(time_t *timer);
time_t mktime(struct tm *tm);
char *asctime(struct tm *tm);
char *ctime(time_t *timer);

/* MEGA65-specific: read RTC directly into struct tm */
void rtc_read(struct tm *tm);
