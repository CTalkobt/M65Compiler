// Comprehensive test for Bug #179: struct tm functions linked from library
// Tests various mktime operations to ensure correctness with linked library
#include <time.h>

volatile char *result_addr = (char *)0x4000;

int main() {
    struct tm t;
    time_t result1, result2, result3;
    int test_num = 0;

    // Test 1: Simple mktime call
    test_num = 1;
    t.tm_year = 126;  // 2026
    t.tm_mon = 6;     // July
    t.tm_mday = 3;
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;
    result1 = mktime(&t);
    result_addr[0] = (result1 != 0) ? 0xAA : 0x01;

    // Test 2: Different date
    test_num = 2;
    t.tm_year = 125;  // 2025
    t.tm_mon = 0;     // January
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    result2 = mktime(&t);
    result_addr[1] = (result2 != 0) ? 0xBB : 0x02;

    // Test 3: Another date
    test_num = 3;
    t.tm_year = 124;  // 2024
    t.tm_mon = 11;    // December
    t.tm_mday = 31;
    t.tm_hour = 23;
    t.tm_min = 59;
    t.tm_sec = 59;
    result3 = mktime(&t);
    result_addr[2] = (result3 != 0) ? 0xCC : 0x03;

    // Test 4: Verify results are different (not all same value)
    int different = (result1 != result2) && (result2 != result3);
    result_addr[3] = different ? 0xDD : 0x04;

    // Test 5: Test with struct members that need to be calculated
    test_num = 5;
    struct tm t2;
    t2.tm_year = 126;
    t2.tm_mon = 6;
    t2.tm_mday = 3;
    t2.tm_hour = 12;
    t2.tm_min = 30;
    t2.tm_sec = 45;
    t2.tm_wday = 0;
    t2.tm_yday = 0;
    t2.tm_isdst = 0;
    time_t result4 = mktime(&t2);
    result_addr[4] = (result4 != 0) ? 0xEE : 0x05;

    // Test 6: Sequential calls (test state isolation)
    test_num = 6;
    struct tm t3;
    t3.tm_year = 126;
    t3.tm_mon = 0;
    t3.tm_mday = 1;
    t3.tm_hour = 0;
    t3.tm_min = 0;
    t3.tm_sec = 0;
    t3.tm_wday = 0;
    t3.tm_yday = 0;
    t3.tm_isdst = 0;
    time_t r1 = mktime(&t3);

    struct tm t4;
    t4.tm_year = 126;
    t4.tm_mon = 0;
    t4.tm_mday = 2;
    t4.tm_hour = 0;
    t4.tm_min = 0;
    t4.tm_sec = 0;
    t4.tm_wday = 0;
    t4.tm_yday = 0;
    t4.tm_isdst = 0;
    time_t r2 = mktime(&t4);

    result_addr[5] = (r1 != 0 && r2 != 0 && r2 > r1) ? 0xFF : 0x06;

    // Completion marker
    result_addr[6] = 0xAA;

    return 0;
}
