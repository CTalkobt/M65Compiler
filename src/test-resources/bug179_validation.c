// Bug #179 Validation Test
// Validates: struct tm functions work correctly when linked from library
// This test verifies the fix for frame allocation conflicts

#include <time.h>
#include <stdio.h>

volatile char *test_result = (char *)0x4000;

void test_mktime() {
    struct tm t;

    // Test 1: Basic mktime call
    t.tm_year = 126;  // 2026 - 1900
    t.tm_mon = 6;     // July (0-11)
    t.tm_mday = 6;    // Day 6
    t.tm_hour = 12;   // 12:00
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    time_t result1 = mktime(&t);

    // Test 2: Different date
    t.tm_year = 125;  // 2025
    t.tm_mon = 0;     // January
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;

    time_t result2 = mktime(&t);

    // Test 3: Leap year date
    t.tm_year = 124;  // 2024 (leap year)
    t.tm_mon = 1;     // February
    t.tm_mday = 29;   // Feb 29

    time_t result3 = mktime(&t);

    // Validation: results should be non-zero and different
    test_result[0] = (result1 != 0) ? 0xAA : 0x01;  // Test 1 result
    test_result[1] = (result2 != 0) ? 0xAA : 0x02;  // Test 2 result
    test_result[2] = (result3 != 0) ? 0xAA : 0x03;  // Test 3 result
    test_result[3] = (result1 != result2) ? 0xAA : 0x04;  // Different dates

    // Marker for completion
    test_result[4] = 0xFF;
}

void main() {
    test_mktime();
}
