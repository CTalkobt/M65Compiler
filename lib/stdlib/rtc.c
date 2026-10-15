/* rtc.c — MEGA65 Real-Time Clock and calendar time functions */

#include <time.h>
#include <string.h>
#include <dma.h>

/* MEGA65 RTC I2C registers at $FFD71xx (directly mapped)
 * BCD-encoded per VHDL (mega65r3_i2c.vhdl / mega65r4_i2c.vhdl):
 *   $FFD7110: seconds (BCD)
 *   $FFD7111: minutes (BCD)
 *   $FFD7112: hours (BCD, 24h mode)
 *   $FFD7113: day of month (BCD)
 *   $FFD7114: month (BCD)
 *   $FFD7115: year (BCD, 00-99)
 *   $FFD7116: day of week (1-7)
 */

static int bcd_to_int(unsigned char bcd) {
    return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

static unsigned char int_to_bcd(int val) {
    return (unsigned char)(((val / 10) << 4) | (val % 10));
}

/* Days in each month (non-leap year) */
static int days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

static int is_leap_year(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

/* Compute day of year (0-based) from month and day */
static int day_of_year(int year, int mon, int mday) {
    int doy = 0;
    int i;
    for (i = 0; i < mon; i++) {
        doy += days_in_month[i];
        if (i == 1 && is_leap_year(year)) doy++;
    }
    return doy + mday - 1;
}

/* Compute day of week using Tomohiko Sakamoto's algorithm (0=Sunday) */
static int compute_wday(int year, int mon, int mday) {
    static int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (mon < 2) year--;
    return (year + year/4 - year/100 + year/400 + t[mon] + mday) % 7;
}

/* Static buffer for localtime/gmtime */
static struct tm __tm_buf;

/* Read the RTC registers: they are at a 28-bit address (a 16-bit pointer
 * cannot reach them), so a DMA job copies the seven bytes */
void rtc_read(struct tm *tm) {
    unsigned char r[7];   /* sec, min, hour, day, month, year, day of week */
    __dma_copy_far((unsigned long)(unsigned int)r, 0xFFD7110UL, 7);

    tm->tm_sec  = bcd_to_int(r[0] & 0x7F);
    tm->tm_min  = bcd_to_int(r[1] & 0x7F);
    tm->tm_hour = bcd_to_int(r[2] & 0x3F);   /* bit 7: the 24-hour mode flag */
    tm->tm_mday = bcd_to_int(r[3] & 0x3F);
    tm->tm_mon  = bcd_to_int(r[4] & 0x1F) - 1; /* 0-based */
    tm->tm_year = bcd_to_int(r[5]) + 100; /* years since 1900; RTC year 00-99 → 2000-2099 */
    /* the weekday from the date (the register's numbering is not the same
     * on every board and in emulators) */
    tm->tm_wday = compute_wday(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
    tm->tm_yday = day_of_year(tm->tm_year + 1900, tm->tm_mon, tm->tm_mday);
    tm->tm_isdst = 0;
}

struct tm *localtime(time_t *timer) {
    /* On MEGA65, local time = RTC time (no timezone support) */
    rtc_read(&__tm_buf);
    return &__tm_buf;
}

struct tm *gmtime(time_t *timer) {
    /* Same as localtime (no timezone) */
    rtc_read(&__tm_buf);
    return &__tm_buf;
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
    tm->tm_wday = compute_wday(full_year, tm->tm_mon, tm->tm_mday);

    return days * 86400L + tm->tm_hour * 3600L + tm->tm_min * 60L + tm->tm_sec;
}

/* Static buffer for asctime/ctime output */
static char __asc_buf[26];

static char *day_names[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char *mon_names[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                               "Jul","Aug","Sep","Oct","Nov","Dec"};

/* Format: "Wed Jun 30 21:49:08 2026\n\0" */
char *asctime(struct tm *tm) {
    char *p = __asc_buf;
    char *dn = day_names[tm->tm_wday % 7];
    char *mn = mon_names[tm->tm_mon % 12];
    int year = tm->tm_year + 1900;

    /* Day name */
    *p++ = dn[0]; *p++ = dn[1]; *p++ = dn[2]; *p++ = ' ';
    /* Month name */
    *p++ = mn[0]; *p++ = mn[1]; *p++ = mn[2]; *p++ = ' ';
    /* Day of month */
    *p++ = (tm->tm_mday / 10) ? ('0' + tm->tm_mday / 10) : ' ';
    *p++ = '0' + tm->tm_mday % 10;
    *p++ = ' ';
    /* Time */
    *p++ = '0' + tm->tm_hour / 10; *p++ = '0' + tm->tm_hour % 10; *p++ = ':';
    *p++ = '0' + tm->tm_min / 10; *p++ = '0' + tm->tm_min % 10; *p++ = ':';
    *p++ = '0' + tm->tm_sec / 10; *p++ = '0' + tm->tm_sec % 10; *p++ = ' ';
    /* Year */
    *p++ = '0' + (year / 1000) % 10;
    *p++ = '0' + (year / 100) % 10;
    *p++ = '0' + (year / 10) % 10;
    *p++ = '0' + year % 10;
    *p++ = '\n';
    *p = 0;
    return __asc_buf;
}

char *ctime(time_t *timer) {
    return asctime(localtime(timer));
}
