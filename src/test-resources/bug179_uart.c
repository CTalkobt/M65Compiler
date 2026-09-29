// Bug #179 UART Validation Test
// Validates: struct tm functions work correctly when linked from library
// Uses UART serial output for real-time status tracking

#include <time.h>
#include <stdio.h>

// UART output via serialtcp
#define UART_DATA_REG 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA_REG;
    *uart = c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

void uart_put_hex(unsigned char val) {
    const char *hex = "0123456789ABCDEF";
    uart_putchar(hex[(val >> 4) & 0xF]);
    uart_putchar(hex[val & 0xF]);
}

void uart_put_hex32(unsigned long val) {
    uart_put_hex((val >> 24) & 0xFF);
    uart_put_hex((val >> 16) & 0xFF);
    uart_put_hex((val >> 8) & 0xFF);
    uart_put_hex(val & 0xFF);
}

void test_mktime() {
    uart_puts("TEST_START\n");

    struct tm t;

    // Test 1: Basic mktime call
    uart_puts("TEST1_SETUP: year=126 mon=6 mday=6\n");
    t.tm_year = 126;  // 2026 - 1900
    t.tm_mon = 6;     // July (0-11)
    t.tm_mday = 6;    // Day 6
    t.tm_hour = 12;   // 12:00
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    uart_puts("TEST1_CALL: mktime(&t)\n");
    time_t result1 = mktime(&t);
    uart_puts("TEST1_RESULT: ");
    uart_put_hex32(result1);
    uart_puts("\n");

    // Test 2: Different date
    uart_puts("TEST2_SETUP: year=125 mon=0 mday=1\n");
    t.tm_year = 125;  // 2025
    t.tm_mon = 0;     // January
    t.tm_mday = 1;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;

    uart_puts("TEST2_CALL: mktime(&t)\n");
    time_t result2 = mktime(&t);
    uart_puts("TEST2_RESULT: ");
    uart_put_hex32(result2);
    uart_puts("\n");

    // Test 3: Leap year date
    uart_puts("TEST3_SETUP: year=124 mon=1 mday=29 (leap year)\n");
    t.tm_year = 124;  // 2024 (leap year)
    t.tm_mon = 1;     // February
    t.tm_mday = 29;   // Feb 29

    uart_puts("TEST3_CALL: mktime(&t)\n");
    time_t result3 = mktime(&t);
    uart_puts("TEST3_RESULT: ");
    uart_put_hex32(result3);
    uart_puts("\n");

    // Validation: results should be non-zero and different
    uart_puts("VALIDATE1: result1 != 0 ? ");
    unsigned char test1 = (result1 != 0) ? 0xAA : 0x01;
    uart_put_hex(test1);
    uart_puts("\n");

    uart_puts("VALIDATE2: result2 != 0 ? ");
    unsigned char test2 = (result2 != 0) ? 0xAA : 0x02;
    uart_put_hex(test2);
    uart_puts("\n");

    uart_puts("VALIDATE3: result3 != 0 ? ");
    unsigned char test3 = (result3 != 0) ? 0xAA : 0x03;
    uart_put_hex(test3);
    uart_puts("\n");

    uart_puts("VALIDATE4: result1 != result2 ? ");
    unsigned char test4 = (result1 != result2) ? 0xAA : 0x04;
    uart_put_hex(test4);
    uart_puts("\n");

    // Summary
    if (test1 == 0xAA && test2 == 0xAA && test3 == 0xAA && test4 == 0xAA) {
        uart_puts("TEST_PASS\n");
    } else {
        uart_puts("TEST_FAIL\n");
    }
}

void main() {
    test_mktime();
    uart_puts("TEST_END\n");
    return 0;
}
