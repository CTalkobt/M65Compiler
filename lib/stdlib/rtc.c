/* rtc.c — MEGA65 Real-Time Clock and calendar time functions
 *
 * time_t counts seconds from 2000-01-01 00:00:00 (signed: 1931 to 2068).
 * The conversions use Howard Hinnant's days-from-civil algorithms, exact
 * for any date.
 */

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

static unsigned char bcd_to_int(unsigned char bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

/* Days from 2000-01-01 to y-m-d (m 1-12) */
static long days_from_civil(long y, int m, int d) {
    long era, yoe, doy, doe;
    if (m <= 2) y--;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = y - era * 400;
    doy = (153L * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097L + doe - 730425L;   /* 730425: 0000-03-01 to 2000-01-01 */
}

/* The date and time of t seconds after 2000-01-01 into *tm */
static void tm_from_time(time_t t, struct tm *tm) {
    long days = t / 86400L;
    long secs = t % 86400L;
    long z, era, doe, yoe, y, doy, mp;
    int m, d;
    if (secs < 0) { secs += 86400L; days--; }
    tm->tm_hour = (int)(secs / 3600);
    tm->tm_min = (int)(secs / 60 % 60);
    tm->tm_sec = (int)(secs % 60);
    /* 2000-01-01 was a Saturday */
    tm->tm_wday = (int)((days % 7 + 13) % 7);
    z = days + 730425L;
    era = (z >= 0 ? z : z - 146096L) / 146097L;
    doe = z - era * 146097L;
    yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096L) / 365;
    y = yoe + era * 400;
    doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    mp = (5 * doy + 2) / 153;
    d = (int)(doy - (153 * mp + 2) / 5 + 1);
    m = (int)(mp < 10 ? mp + 3 : mp - 9);
    if (m <= 2) y++;
    tm->tm_year = (int)(y - 1900);
    tm->tm_mon = m - 1;
    tm->tm_mday = d;
    tm->tm_yday = (int)(days - days_from_civil(y, 1, 1));
    tm->tm_isdst = 0;
}

/* Static buffer for localtime/gmtime */
static struct tm __tm_buf;

/* Read the RTC registers: they are at a 28-bit address (a 16-bit pointer
 * cannot reach them), so a DMA job copies the seven bytes. Only the fields
 * the registers hold: tm_wday and tm_yday are -1 (mktime fills them in),
 * so a program reading the clock links none of the date arithmetic (#355) */
void rtc_read(struct tm *tm) {
    unsigned char r[7];   /* sec, min, hour, day, month, year, day of week */
    __dma_copy_far((unsigned long)(unsigned int)r, 0xFFD7110UL, 7);

    tm->tm_sec  = bcd_to_int(r[0] & 0x7F);
    tm->tm_min  = bcd_to_int(r[1] & 0x7F);
    tm->tm_hour = bcd_to_int(r[2] & 0x3F);   /* bit 7: the 24-hour mode flag */
    tm->tm_mday = bcd_to_int(r[3] & 0x3F);
    tm->tm_mon  = bcd_to_int(r[4] & 0x1F) - 1; /* 0-based */
    tm->tm_year = bcd_to_int(r[5]) + 100; /* years since 1900; RTC year 00-99 → 2000-2099 */
    /* (the weekday register's numbering is not the same on every board
     * and in emulators: not used) */
    tm->tm_wday = -1;
    tm->tm_yday = -1;
    tm->tm_isdst = 0;
}

struct tm *gmtime(time_t *timer) {
    tm_from_time(*timer, &__tm_buf);
    return &__tm_buf;
}

/* The RTC keeps local time and there are no time zones: localtime is gmtime */
struct tm *localtime(time_t *timer) {
    return gmtime(timer);
}

/* struct tm to time_t. Fields out of range are carried (tm_mon 12 is January
 * of the next year, tm_mday 0 the last day of the previous month, negative
 * values count back), and *tm is rewritten normalized with tm_wday/tm_yday */
time_t mktime(struct tm *tm) {
    long y = tm->tm_year + 1900L;
    long mon = tm->tm_mon;
    time_t t;
    y += mon / 12;
    mon %= 12;
    if (mon < 0) { mon += 12; y--; }
    t = days_from_civil(y, (int)mon + 1, 1) + tm->tm_mday - 1;
    t = t * 86400L + tm->tm_hour * 3600L + tm->tm_min * 60L + tm->tm_sec;
    tm_from_time(t, tm);
    return t;
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
