// Test mktime with minimal library
__asm__(".no_zp_save");

#include <time.h>

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

void main() {
    uart_putchar('A');

    struct tm t;
    t.tm_year = 126;
    t.tm_mon = 6;
    t.tm_mday = 6;
    t.tm_hour = 12;
    t.tm_min = 0;
    t.tm_sec = 0;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    uart_putchar('B');

    time_t result = mktime(&t);

    uart_putchar('C');
    uart_putchar('\n');

    return 0;
}
