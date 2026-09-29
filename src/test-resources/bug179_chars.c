// Test: Use only uart_putchar, not uart_puts
__asm__(".no_zp_save");

#include <time.h>

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

void main() {
    // Just print single characters
    uart_putchar('A');  // 0x41
    uart_putchar('B');  // 0x42
    uart_putchar('\n');

    struct tm t;
    uart_putchar('C');
    uart_putchar('\n');

    t.tm_year = 0;
    uart_putchar('D');
    uart_putchar('\n');

    t.tm_year = 126;
    uart_putchar('E');
    uart_putchar('\n');

    uart_putchar('F');
    uart_putchar('\n');
    return 0;
}
