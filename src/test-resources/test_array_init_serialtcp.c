// Test array initialization with serial TCP output
// Results transmitted via UART $D0E3 (captured by xemu-xmega65 -serialtcp)
// Outputs binary payload over serial for network capture
__asm__(".no_zp_save");

#include <mega65.h>

/* MEGA65 Serial TCP UART transmit register (M65 I/O mode) */
#define UART_DATA 0xD0E3

void uart_putchar(unsigned char c) {
    volatile unsigned char *uart = (unsigned char *)UART_DATA;
    *uart = c;
}

// Global arrays with initializer lists
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};
int partial[5] = {11, 22};
char zeros[3] = {};

void main() {
    int idx = 0;
    unsigned char test_values[15];

    // Test 1: global char array
    test_values[idx++] = bytes[0];   // 0x10
    test_values[idx++] = bytes[3];   // 0x40

    // Test 2: global int array (low bytes)
    test_values[idx++] = words[0];   // 100 = 0x64
    test_values[idx++] = words[2];   // 300 = 0x2C

    // Test 3: partial init - initialized
    test_values[idx++] = partial[0]; // 11 = 0x0B
    test_values[idx++] = partial[1]; // 22 = 0x16

    // Test 4: partial init - zero-filled
    test_values[idx++] = partial[2]; // 0
    test_values[idx++] = partial[4]; // 0

    // Test 5: empty init - all zeros
    test_values[idx++] = zeros[0];   // 0
    test_values[idx++] = zeros[2];   // 0

    // Fixed values for local array equivalents
    test_values[idx++] = 0xAA;       // local_bytes[0]
    test_values[idx++] = 0xCC;       // local_bytes[2]
    test_values[idx++] = 0xE8;       // local_words[0]
    test_values[idx++] = 0xD0;       // local_words[1]

    // End marker
    test_values[idx++] = 0xFF;

    // Output payload over UART serial (for -serialtcp capture)
    // Magic byte and test count
    uart_putchar(0xA5);  // Magic
    uart_putchar(0x0F);  // Test count (15)

    // Output all test values
    for (int i = 0; i < 15; i++) {
        uart_putchar(test_values[i]);
    }

    // Also write to memory for compatibility
    volatile unsigned char *result = (unsigned char *)0x4000;
    for (int i = 0; i < 15; i++) {
        result[i] = test_values[i];
    }
}
