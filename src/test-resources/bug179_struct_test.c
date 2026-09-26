// Test: Can we allocate and use struct tm at all?
__asm__(".no_zp_save");

#include <time.h>

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putchar(*s++);
    }
}

void main() {
    uart_puts("START\n");

    // Declare struct tm
    struct tm t;
    uart_puts("ALLOCATED\n");

    // Try simple assignment with constant 0
    t.tm_year = 0;
    uart_puts("SET_ZERO\n");

    // Try assignment with small constant
    t.tm_year = 1;
    uart_puts("SET_ONE\n");

    // Try with larger constant
    t.tm_year = 100;
    uart_puts("SET_100\n");

    // Try with 126 (the problematic value from bug179)
    t.tm_year = 126;
    uart_puts("SET_126\n");

    // If we got here, it works!
    uart_puts("SUCCESS\n");
}
