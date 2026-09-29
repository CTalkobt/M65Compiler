// Test array initialization with braced initializer lists
// Validates global and local array initializers, partial init, empty init.
//
// NOTE: global initialized arrays may fail until #262 is fixed (DATA vs BSS).
// Expected at $C010: 10 40 64 2C 0B 16 00 00 00 00 AA
//   [0]=bytes[0], [1]=bytes[3], [2]=words[0]lo, [3]=words[2]lo,
//   [4]=partial[0]lo, [5]=partial[1]lo, [6]=partial[2]lo, [7]=partial[4]lo,
//   [8]=zeros[0], [9]=zeros[2], [10]=marker

#define RESULT(i) (*(volatile unsigned char *)(0xC010 + (i)))

// Global arrays with initializer lists
char bytes[4] = {0x10, 0x20, 0x30, 0x40};
int words[3] = {100, 200, 300};

// Partial initialization (remaining elements zero-filled)
int partial[5] = {11, 22};

// Empty initializer list (all zeros)
char zeros[3] = {};

void main() {
    // Test 1: global char array
    RESULT(0) = bytes[0];   // 0x10
    RESULT(1) = bytes[3];   // 0x40

    // Test 2: global int array (low bytes)
    RESULT(2) = (unsigned char)words[0];   // 100 = 0x64
    RESULT(3) = (unsigned char)words[2];   // 300 = 0x2C (low byte of 0x012C)

    // Test 3: partial init - initialized elements
    RESULT(4) = (unsigned char)partial[0]; // 11 = 0x0B
    RESULT(5) = (unsigned char)partial[1]; // 22 = 0x16

    // Test 4: partial init - zero-filled elements
    RESULT(6) = (unsigned char)partial[2]; // 0
    RESULT(7) = (unsigned char)partial[4]; // 0

    // Test 5: empty init - all zeros
    RESULT(8) = zeros[0];   // 0
    RESULT(9) = zeros[2];   // 0

    RESULT(10) = 0xAA;      // marker
}
