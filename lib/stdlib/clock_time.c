/* clock_time.c — clock_gettime, clock_getres, clock_settime, tzset
 * (<time.h>)
 *
 * CLOCK_REALTIME is the RTC (time(): whole seconds since 2000-01-01);
 * CLOCK_MONOTONIC the KERNAL's frame counter (clock()). clock_settime
 * writes the RTC's registers at $FFD7110 (BCD; the hours with bit 7, the
 * 24-hour mode) by DMA.
 */

#include <time.h>
#include <errno.h>
#include <dma.h>

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
    clock_t f;
    unsigned int rate;
    if (clock_id == CLOCK_REALTIME) {
        tp->tv_sec = time(NULL);
        tp->tv_nsec = 0;
        return 0;
    }
    if (clock_id == CLOCK_MONOTONIC) {
        f = clock();
        rate = (unsigned int)CLOCKS_PER_SEC;
        tp->tv_sec = (time_t)(f / rate);
        tp->tv_nsec = (signed long)(f % rate) * (1000000000L / rate);
        return 0;
    }
    errno = EINVAL;
    return -1;
}

int clock_getres(clockid_t clock_id, struct timespec *res) {
    if (clock_id == CLOCK_REALTIME) {
        res->tv_sec = 1;
        res->tv_nsec = 0;
        return 0;
    }
    if (clock_id == CLOCK_MONOTONIC) {
        res->tv_sec = 0;
        res->tv_nsec = 1000000000L / (unsigned int)CLOCKS_PER_SEC;
        return 0;
    }
    errno = EINVAL;
    return -1;
}

static unsigned char to_bcd(signed int v) {
    return (unsigned char)(((v / 10) << 4) | (v % 10));
}

int clock_settime(clockid_t clock_id, const struct timespec *tp) {
    struct tm *tm;
    time_t t;
    unsigned char r[6];
    if (clock_id != CLOCK_REALTIME || tp->tv_nsec < 0 || tp->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }
    t = tp->tv_sec;
    tm = gmtime(&t);
    if (tm->tm_year < 100 || tm->tm_year > 199) {
        errno = EINVAL;
        return -1;
    }
    r[0] = to_bcd(tm->tm_sec);
    r[1] = to_bcd(tm->tm_min);
    r[2] = to_bcd(tm->tm_hour) | 0x80;
    r[3] = to_bcd(tm->tm_mday);
    r[4] = to_bcd(tm->tm_mon + 1);
    r[5] = to_bcd(tm->tm_year - 100);
    __dma_copy_far(0xFFD7110UL, (unsigned long)(unsigned int)r, 6);
    return 0;
}

void tzset(void) {
}
