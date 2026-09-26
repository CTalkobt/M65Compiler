// Test array initialization with direct UART output
// Results transmitted via UART (captured by xemu-xmega65 -serialtcp)
// Writes raw bytes directly to UART data register at $D600
__asm__(".no_zp_save");

#include <mega65.h>

// UART registers at $D600
#define UART_DATA    (*(volatile unsigned char *)0xD600)
#define UART_STATUS  (*(volatile unsigned char *)0xD601)
#define UART_BUSY    0x01

// Global arrays with initializer lists
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};
int partial[5] = {11, 22};
char zeros[3] = {};

void uart_write_byte(unsigned char byte) {
    // Wait for UART to be ready
    int timeout = 10000;
    while ((UART_STATUS & UART_BUSY) && timeout-- > 0) {
        // Busy wait
    }
    UART_DATA = byte;
}

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

    // Output payload over UART (for -serialtcp capture)
    uart_write_byte(0xA5);  // Magic
    uart_write_byte(0x0F);  // Test count (15)

    // Output all test values
    for (int i = 0; i < 15; i++) {
        uart_write_byte(test_values[i]);
    }

    // Flush (write to memory as marker)
    volatile unsigned char *result = (unsigned char *)0x4000;
    result[0] = 0xA5;  // Mark completion in memory
}
