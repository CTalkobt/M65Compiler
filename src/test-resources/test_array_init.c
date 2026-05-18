// Test array initialization with braced initializer lists
// Results written to $4000+ for mmemu validation
// Disable ZP save — 105-byte frame + 248-byte ZP save overflows 256-byte page-1 stack
__asm__(".no_zp_save");

volatile char *result = (char *)0x4000;

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
    result[0] = bytes[0];   // 0x10
    result[1] = bytes[3];   // 0x40

    // Test 2: global int array (low bytes)
    result[2] = words[0];   // 100 = 0x64
    result[3] = words[2];   // 300 = 0x2C (low byte of 0x012C)

    // Test 3: partial init - initialized elements
    result[4] = partial[0]; // 11 = 0x0B
    result[5] = partial[1]; // 22 = 0x16

    // Test 4: partial init - zero-filled elements
    result[6] = partial[2]; // 0
    result[7] = partial[4]; // 0

    // Test 5: empty init - all zeros
    result[8] = zeros[0];   // 0
    result[9] = zeros[2];   // 0

    // Test 6: local char array
    result[10] = local_bytes[0]; // 0xAA
    result[11] = local_bytes[2]; // 0xCC

    // Test 7: local int array (low bytes)
    result[12] = local_words[0]; // 1000 = 0xE8 (low byte of 0x03E8)
    result[13] = local_words[1]; // 2000 = 0xD0 (low byte of 0x07D0)

    // Marker
    result[14] = 0xFF;
}
