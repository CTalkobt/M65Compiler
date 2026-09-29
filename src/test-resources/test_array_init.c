// Test array initialization with braced initializer lists
// Results transmitted via UART $D0E3 to xemu -serialtcp listener
// Disable ZP save — 105-byte frame + 248-byte ZP save overflows 256-byte page-1 stack
__asm__(".no_zp_save");

#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

// Global arrays with initializer lists
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};

// Partial initialization (remaining elements zero-filled)
int partial[5] = {11, 22};

// Empty initializer list (all zeros)
char zeros[3] = {};

void main() {
    // Local array with initializer list
    char local_bytes[3] = {0xAA, 0xBB, 0xCC};
    int local_words[2] = {1000, 2000};

    // Test 1: global char array
    uart_putchar(bytes[0]);   // 0x10
    uart_putchar(bytes[3]);   // 0x40

    // Test 2: global int array (low bytes)
    uart_putchar(words[0]);   // 100 = 0x64
    uart_putchar(words[2]);   // 300 = 0x2C (low byte of 0x012C)

    // Test 3: partial init - initialized elements
    uart_putchar(partial[0]); // 11 = 0x0B
    uart_putchar(partial[1]); // 22 = 0x16

    // Test 4: partial init - zero-filled elements
    uart_putchar(partial[2]); // 0
    uart_putchar(partial[4]); // 0

    // Test 5: empty init - all zeros
    uart_putchar(zeros[0]);   // 0
    uart_putchar(zeros[2]);   // 0

    // Test 6: local char array
    uart_putchar(local_bytes[0]); // 0xAA
    uart_putchar(local_bytes[2]); // 0xCC

    // Test 7: local int array (low bytes)
    uart_putchar(local_words[0]); // 1000 = 0xE8 (low byte of 0x03E8)
    uart_putchar(local_words[1]); // 2000 = 0xD0 (low byte of 0x07D0)

    // Terminator
    uart_putchar(0xFF);
}
