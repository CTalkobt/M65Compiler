// mmemu validation test for long type (32-bit)
// Expected bytes at $4000: 04 C0 01 A0 2A A0 00
// Expected bytes at $4007: E0 93 04 00 AA
// Converted to UART serialtcp validation test
// Results transmitted via UART $D0E3 to xemu -serialtcp listener

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}


long add_longs(long a, long b) {
    return a + b;
}

long global_a = 100000L;
long global_b = 200000L;
unsigned long global_c;

void main() {
    __asm__(".no_zp_save");
    // sizeof(long) = 4
    uart_putchar(sizeof(long));

    // 50000+70000=120000=$1D4C0, low byte=$C0
    long sum = add_longs(50000L, 70000L);
    uart_putchar((char)sum);

    // comparison: 100000 < 200000
    if (global_a < global_b)
        uart_putchar(1);
    else
        uart_putchar(0);

    // low byte of 100000 ($186A0) = $A0
    uart_putchar((char)global_a);

    // cast int -> long -> truncate: 42
    int small = 42;
    long big = (long)small;
    uart_putchar((char)big);

    // cast long -> int (narrowing), low byte of $186A0 = $A0
    int narrow = (int)global_a;
    uart_putchar((char)narrow);

    // overflow test: 0xFFFFFFFF + 1 = 0
    global_c = 0xFFFFFFFFL;
    global_c++;
    uart_putchar((char)global_c);

    // global = global_a + global_b: 100000 + 200000 = 300000 = $493E0
    // verify all 4 bytes
    global_c = global_a + global_b;
    uart_putchar((char)global_c);          // $E0
    uart_putchar((char)(global_c >> 8));   // $93
    uart_putchar((char)(global_c >> 16));  // $04
    uart_putchar((char)(global_c >> 24)); // $00

    uart_putchar(0xAA);  // marker
}
