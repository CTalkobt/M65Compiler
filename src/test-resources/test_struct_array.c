
// Converted to UART serialtcp validation test
// Results transmitted via UART $D0E3 to xemu -serialtcp listener

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

struct Point {
    int x;
    int y;
};

struct Point pts[4] = 0;

void main() {
    int i;

    // Fill with loop
    for (i = 0; i < 4; i++) {
        pts[i].x = i * 10;
        pts[i].y = i * 10 + 1;
    }

    // Read back
    uart_putchar(pts[0].x);   // 0
    uart_putchar(pts[1].x);   // 10 = $0A
    uart_putchar(pts[2].y);   // 21 = $15
    uart_putchar(pts[3].y);   // 31 = $1F
    uart_putchar(sizeof(pts)); // 4 * 4 = 16 = $10
    uart_putchar(0xAA);
}
