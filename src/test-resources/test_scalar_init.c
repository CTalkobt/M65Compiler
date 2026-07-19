// Test: Scalar initializer frame offset correctness
// Validates: Local scalar variables are correctly initialized and stored to proper frame offsets,
//            even when multiple local variables exist with different types and sizes.
// This ensures that scalar initializers compute correct frame addresses before storing.

volatile char *result = (char *)0x4000;

void test_scalars() {
    // Multiple local variables to test frame offset allocation
    short a = 10;           // Should be at frame offset for 'a'
    char b = 20;            // Should be at frame offset for 'b'
    short c = 30;           // Should be at frame offset for 'c'
    int d = 40;             // Should be at frame offset for 'd'

    // Write to result buffer to verify values were stored correctly
    result[0] = a;          // Low byte of 10 = 0x0A
    result[1] = (a >> 8);   // High byte of 10 = 0x00
    result[2] = b;          // 20 = 0x14
    result[3] = c;          // Low byte of 30 = 0x1E
    result[4] = (c >> 8);   // High byte of 30 = 0x00
    result[5] = d;          // Low byte of 40 = 0x28
    result[6] = (d >> 8);   // High byte of 40 = 0x00
    result[7] = 0xAA;       // Marker

    __asm__("brk");         // Signal completion
}

void test_scalar_assign() {
    // Test that scalars initialized with expressions work correctly
    short x = 5 + 15;       // Expression initializer
    char y = 7 * 3;         // Expression initializer (21)

    result[0] = x;          // 20 = 0x14
    result[1] = (x >> 8);   // 0x00
    result[2] = y;          // 21 = 0x15
    result[3] = 0xBB;       // Marker

    __asm__("brk");
}

void test_scalar_after_call(short (*helper)(void)) {
    // Test that scalars maintain correct values after function calls
    // This is critical: the fix ensures ADDR_LOCAL is used even after calls
    short before = 100;
    short after_call = helper();
    short after = 200;

    result[0] = before;     // 100 = 0x64
    result[1] = (before >> 8);  // 0x00
    result[2] = after_call & 0xFF;     // From helper
    result[3] = after;      // 200 = 0xC8
    result[4] = (after >> 8);   // 0x00
    result[5] = 0xCC;       // Marker

    __asm__("brk");
}

short helper_func(void) {
    return 42;
}
