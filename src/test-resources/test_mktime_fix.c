// Test mktime with the function pointer fix
#include <time.h>

volatile char *test_result = (char *)0x4000;
int test_count = 0;

void main() {
    struct tm t;
    time_t result;

    // Test 1: Simple date
    t.tm_year = 126;  // 2026 - 1900
    t.tm_mon = 6;     // July (0-11)
    t.tm_mday = 3;    // Day 3
    t.tm_hour = 12;   // 12:00
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    result = mktime(&t);

    // Mark test 1 complete
    test_result[0] = 0xAA;

    // Store result bytes
    test_result[1] = (result >> 0) & 0xFF;
    test_result[2] = (result >> 8) & 0xFF;
    test_result[3] = (result >> 16) & 0xFF;
    test_result[4] = (result >> 24) & 0xFF;

    // If result is non-zero, mktime worked
    test_result[5] = (result != 0) ? 0xBB : 0xCC;

    // Completion marker
    test_result[6] = 0xFF;
}
