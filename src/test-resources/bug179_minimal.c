// Minimal test - just try to assign to struct member
#include <time.h>

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
    uart_puts("A");  // Check if we get here

    struct tm t;
    uart_puts("B");  // Check if we get here

    // Try setting just one field
    t.tm_year = 0;
    uart_puts("C");  // Check if we get here

    t.tm_year = 126;
    uart_puts("D");  // Check if we get here

    uart_puts("DONE");
}
