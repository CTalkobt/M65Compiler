// Most basic test - just print one character
__asm__(".no_zp_save");

#define UART_DATA_REG 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA_REG;
    *uart = c;
}

void main() {
    uart_putchar('X');
}
