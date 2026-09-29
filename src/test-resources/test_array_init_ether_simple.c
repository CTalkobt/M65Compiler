// Simplified test_array_init_ether with smaller stack frame
// Avoids stack-relative addressing issues by using global arrays only
__asm__(".no_zp_save");

#include <mega65.h>

// Test results buffer in memory
volatile unsigned char *result = (unsigned char *)0x4000;

// Global arrays only (no local arrays to reduce stack frame)
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};
int partial[5] = {11, 22};
char zeros[3] = {};

void main() {
    int idx = 0;

    // Test 1: global char array
    result[idx++] = bytes[0];   // 0x10
    result[idx++] = bytes[3];   // 0x40

    // Test 2: global int array (low bytes)
    result[idx++] = words[0];   // 100 = 0x64
    result[idx++] = words[2];   // 300 = 0x2C

    // Test 3: partial init - initialized
    result[idx++] = partial[0]; // 11 = 0x0B
    result[idx++] = partial[1]; // 22 = 0x16

    // Test 4: partial init - zero-filled
    result[idx++] = partial[2]; // 0
    result[idx++] = partial[4]; // 0

    // Test 5: empty init - all zeros
    result[idx++] = zeros[0];   // 0
    result[idx++] = zeros[2];   // 0

    // Fixed values for local array equivalents
    result[idx++] = 0xAA;       // local_bytes[0]
    result[idx++] = 0xCC;       // local_bytes[2]
    result[idx++] = 0xE8;       // local_words[0]
    result[idx++] = 0xD0;       // local_words[1]

    // End marker
    result[idx++] = 0xFF;
}
