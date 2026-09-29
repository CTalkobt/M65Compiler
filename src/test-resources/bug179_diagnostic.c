// Bug #179 Diagnostic Test - Simplified to track mktime hang
// Tracks every step to pinpoint where mktime() gets stuck

#include <time.h>

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

void main() {
    uart_puts("START\n");

    struct tm t;
    uart_puts("ALLOC_TM\n");

    // Initialize all fields to zero first
    t.tm_year = 0;
    t.tm_mon = 0;
    t.tm_mday = 0;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;
    uart_puts("CLEAR_FIELDS\n");

    // Set test 1 values
    t.tm_year = 126;
    uart_puts("SET_YEAR\n");

    t.tm_mon = 6;
    uart_puts("SET_MON\n");

    t.tm_mday = 6;
    uart_puts("SET_MDAY\n");

    t.tm_hour = 12;
    uart_puts("SET_HOUR\n");

    t.tm_min = 0;
    uart_puts("SET_MIN\n");

    t.tm_sec = 0;
    uart_puts("SET_SEC\n");

    uart_puts("CALL_MKTIME\n");
    time_t result1 = mktime(&t);
    uart_puts("MKTIME_RETURNED\n");

    if (result1 == 0) {
        uart_puts("ERROR_ZERO_RESULT\n");
    } else {
        uart_puts("SUCCESS_NONZERO\n");
    }

    uart_puts("DONE\n");
}
