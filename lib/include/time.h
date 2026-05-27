/* time.h — Minimal time functions for cc45 / MEGA65
 *
 * Based on the MEGA65 jiffy clock (60Hz, 24-bit counter via KERNAL RDTIM).
 *
 * Limitations:
 *   - clock() returns 16-bit jiffy count (wraps every ~18 minutes)
 *   - time() returns 32-bit jiffy count (wraps every ~2.3 years)
 *   - No real-time clock support (no date/time, just elapsed ticks)
 *   - CLOCKS_PER_SEC is 60 (NTSC jiffy rate; PAL is ~50 but we use 60)
 */

#pragma once

#define CLOCKS_PER_SEC 60

typedef long time_t;
typedef int clock_t;

clock_t clock(void);
time_t time(time_t *tp);
long difftime(time_t t2, time_t t1);
