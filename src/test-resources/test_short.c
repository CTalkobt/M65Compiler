// Test: short type (alias for int on 16-bit target)
// Validates: short, unsigned short, signed short, short in function params/returns,
//            sizeof(short), short pointers, short arrays.
// Converted to UART serialtcp validation test

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

short add_short(short a, short b) {
    return a + b;
}

unsigned short mul_short(unsigned short a, unsigned short b) {
    return a * b;
}

void main() {
    short x = 10;
    short y = 20;
    unsigned short z = add_short(x, y);
    signed short neg = -5;
    short arr[3] = {100, 200, 300};
    short *p = &x;

    uart_putchar(z);                // 30 = 0x1E
    uart_putchar(neg + 10);         // 5
    uart_putchar(sizeof(short));    // 2
    uart_putchar(mul_short(3, 4));  // 12
    uart_putchar(*p);               // 10 = 0x0A
    uart_putchar(arr[1]);           // 200 = 0xC8
    uart_putchar(0xFF);             // terminator

}
