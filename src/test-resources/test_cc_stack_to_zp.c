// Test: Stack-convention caller → ZP-convention callee (ALLOWED)
// This is the fastcall interop case: stack code calls ZP functions
// Runtime assertions validate fastcall interop correctness

#include <assert.h>

char *results = 0x4000;

// ZP function: uses ZP params
__fastcall__ int zp_add(int a, int b) {
    return a + b;
}

// ZP function with return type variety
__fastcall__ char zp_char_mul(char x, char y) {
    return x * y;
}

// Stack-convention main (default)
void main() {
    // Test 1: Call ZP function from stack code
    int r1 = zp_add(10, 20);
    assert(r1 == 30);
    results[0] = (char)(r1 & 0xFF);      // Should be 30 (0x1E)
    results[1] = (char)((r1 >> 8) & 0xFF);

    // Test 2: Another ZP call
    int r2 = zp_add(100, 55);
    assert(r2 == 155);
    results[2] = (char)(r2 & 0xFF);      // Should be 155 (0x9B)
    results[3] = (char)((r2 >> 8) & 0xFF);

    // Test 3: ZP function returning char
    char r3 = zp_char_mul(5, 7);
    assert(r3 == 35);
    results[4] = r3;                      // Should be 35 (0x23)

    // Sentinel
    results[5] = 0xFF;

    return 0;  // Success
}
