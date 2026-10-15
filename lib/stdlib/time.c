/* time.c — time(): calendar time from the MEGA65 RTC
 *
 * Seconds since 2000-01-01 00:00:00 (the epoch of mktime and gmtime), read
 * from the RTC; the RTC has no time zone, so it is local time.
 */
#include <time.h>

time_t time(time_t *tp) {
    struct tm tm;
    time_t t;
    rtc_read(&tm);
    t = mktime(&tm);
    if (tp) *tp = t;
    return t;
}
