/* time.h — Time functions for cc45 / MEGA65
 *
 * clock(): video frames since power-on (the KERNAL's frame counter);
 * CLOCKS_PER_SEC is the frame rate, 50 (PAL) or 60 (NTSC).
 * Calendar time: time_t is seconds since 2000-01-01 00:00:00. time() reads
 * the MEGA65 RTC ($FFD7110, by DMA; xemu gives the host's time); gmtime,
 * localtime (the same: the RTC has no time zone), mktime, asctime, ctime
 * and strftime convert. clock_gettime reads the RTC (CLOCK_REALTIME, whole
 * seconds) or the frame counter (CLOCK_MONOTONIC); clock_settime sets the
 * RTC.
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

/* MEGA65-specific: read the RTC into a struct tm: the date and time only,
 * tm_wday and tm_yday -1 (call mktime(tm) afterwards for them), tm_isdst 0 */
void rtc_read(struct tm *tm);

/* strftime: tm formatted into s (at most max bytes with the NUL) by the
 * C99 conversions (%a %A %b %B %c %C %d %D %e %F %g %G %h %H %I %j %m %M
 * %n %p %r %R %S %t %T %u %U %V %w %W %x %X %y %Y %z %Z %%; E and O
 * modifiers ignored) in the "C" locale; %z is +0000 and %Z empty (no time
 * zones). The characters written without the NUL, or 0 when they do not
 * fit. */
unsigned int strftime(char *restrict s, unsigned int max, const char *restrict format,
                      const struct tm *restrict tm);

/* No time zones: nothing to set */
void tzset(void);

typedef unsigned char clockid_t;
#define CLOCK_REALTIME  0       /* the RTC: time() and a whole second */
#define CLOCK_MONOTONIC 1       /* clock()'s frames since power-on */

struct timespec {
    time_t tv_sec;
    signed long tv_nsec;
};

/* The clock's time in *tp: 0, or -1 (errno EINVAL) for another clock */
int clock_gettime(clockid_t clock_id, struct timespec *tp);
/* The clock's resolution: a second, or a frame (20 ms PAL, 16.67 ms NTSC) */
int clock_getres(clockid_t clock_id, struct timespec *res);
/* CLOCK_REALTIME: the RTC set to tp's second (2000 to 2099; the RTC's
 * registers, which the I2C controller writes to the clock chip; xemu keeps
 * the host's time). 0, or -1 (errno EINVAL) */
int clock_settime(clockid_t clock_id, const struct timespec *tp);
