// Bug #179: struct tm functions produce wrong results when linked from library
// Test case to reproduce the issue

#include <time.h>
#include <stdio.h>

volatile char *result_addr = (char *)0x4000;

void main() {
    struct tm t;

    // Set a known date: 2026-07-06 12:00:00
    t.tm_year = 126;  // 2026 - 1900
    t.tm_mon = 6;     // July (0-11)
    t.tm_mday = 3;    // Day 3
    t.tm_hour = 12;   // 12:00
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;    // Will be set by mktime
    t.tm_yday = 0;    // Will be set by mktime
    t.tm_isdst = 0;   // Not daylight saving

    // Call mktime - this should NOT return 0
    time_t result = mktime(&t);

    // Store result for inspection
    result_addr[0] = (result >> 0) & 0xFF;
    result_addr[1] = (result >> 8) & 0xFF;
    result_addr[2] = (result >> 16) & 0xFF;
    result_addr[3] = (result >> 24) & 0xFF;

    // Marker for test completion
    result_addr[4] = 0xAA;
}
