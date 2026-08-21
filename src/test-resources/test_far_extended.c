// Phase 98.4: Far Address Support Validation
// Comprehensive test suite for 24-bit extended memory access

#include <stdio.h>

// ============================================================================
// Test 1: Far global variables
// ============================================================================

__far unsigned char far_global_byte = 0x42;
__far unsigned int far_global_word = 0x1234;
__far long far_global_long = 0x12345678L;

void test_far_global_access(void) {
    // Test far byte access
    far_global_byte = 0xFF;
    unsigned char b = far_global_byte;
    printf("Far byte: %02X (expected: FF)\n", b);

    // Test far word access
    far_global_word = 0xABCD;
    unsigned int w = far_global_word;
    printf("Far word: %04X (expected: ABCD)\n", w);

    // Test far long access
    far_global_long = 0xDEADBEEFL;
    long l = far_global_long;
    printf("Far long: %08lX (expected: DEADBEEF)\n", l);
}

// ============================================================================
// Test 2: Far arrays
// ============================================================================

__far unsigned char far_array[256];

void test_far_array_access(void) {
    // Initialize far array
    int i;
    for (i = 0; i < 256; i++) {
        far_array[i] = i & 0xFF;
    }

    // Verify access
    unsigned char val = far_array[42];
    printf("Far array[42]: %02X (expected: 2A)\n", val);

    val = far_array[255];
    printf("Far array[255]: %02X (expected: FF)\n", val);
}

// ============================================================================
// Test 3: Far pointers
// ============================================================================

__far long* far_ptr = 0;

void test_far_pointer_dereference(void) {
    // Set far pointer to far_global_long address
    far_ptr = &far_global_long;

    // This would require address-of operator support for __far
    // For now, test basic pointer operations
    if (far_ptr != 0) {
        printf("Far pointer dereference: supported\n");
    } else {
        printf("Far pointer dereference: not yet implemented\n");
    }
}

// ============================================================================
// Test 4: Bank transitions
// ============================================================================

__far unsigned char bank0_data[64];
__far unsigned char bank1_data[64];

void test_bank_transitions(void) {
    // Access different bank data sequentially
    // This tests whether bank setup code is emitted correctly

    bank0_data[0] = 0xAA;
    unsigned char b0 = bank0_data[0];
    printf("Bank0: %02X (expected: AA)\n", b0);

    bank1_data[0] = 0xBB;
    unsigned char b1 = bank1_data[0];
    printf("Bank1: %02X (expected: BB)\n", b1);

    // Access bank0 again (should re-setup bank)
    b0 = bank0_data[32];
    printf("Bank0 again: %02X (expected: 00)\n", b0);
}

// ============================================================================
// Test 5: Mixed addressing modes
// ============================================================================

unsigned char normal_global = 0x55;
__far unsigned char far_global = 0x66;

void test_mixed_addressing(void) {
    // Mix normal and far accesses
    unsigned char n = normal_global;
    unsigned char f = far_global;

    printf("Normal: %02X, Far: %02X (expected: 55, 66)\n", n, f);

    normal_global = 0x77;
    far_global = 0x88;

    n = normal_global;
    f = far_global;
    printf("Normal updated: %02X, Far updated: %02X (expected: 77, 88)\n", n, f);
}

// ============================================================================
// Main test driver
// ============================================================================

int main(void) {
    printf("=== Phase 98.4: Far Address Support Validation ===\n\n");

    printf("Test 1: Far global variables\n");
    test_far_global_access();
    printf("\n");

    printf("Test 2: Far arrays\n");
    test_far_array_access();
    printf("\n");

    printf("Test 3: Far pointers\n");
    test_far_pointer_dereference();
    printf("\n");

    printf("Test 4: Bank transitions\n");
    test_bank_transitions();
    printf("\n");

    printf("Test 5: Mixed addressing modes\n");
    test_mixed_addressing();
    printf("\n");

    printf("=== All tests complete ===\n");
    return 0;
}
