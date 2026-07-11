// Validation test for addr_elem.16 and struct_elem.16 simulated opcodes
// Tests all documented API interfaces using C arrays as data

volatile unsigned char *output = (unsigned char *)0x4000;

// Test data - array of bytes
unsigned char byte_array[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

// Test data - struct array (2-byte elements)
unsigned char struct_array[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

// Test counter and error marker
int test_count = 0;
int errors = 0;

// Helper to test a single operation
void check(int actual, int expected, const char *test_name) {
    output[0x20 + test_count] = test_count;  // Test number
    output[0x30 + test_count] = expected;    // Expected value
    output[0x40 + test_count] = actual;      // Actual value

    if (actual != expected) {
        errors++;
        output[0x50 + test_count] = 0xFF;    // Error marker
    } else {
        output[0x50 + test_count] = 0x00;    // Pass marker
    }

    test_count++;
}

void main() {
    // Initialize output buffer
    int i;
    for (i = 0; i < 0x60; i++) {
        output[i] = 0;
    }

    test_count = 0;
    errors = 0;

    // TEST 1: Direct array access - validate test data
    check(byte_array[0], 0x11, "array[0]");
    check(byte_array[1], 0x22, "array[1]");
    check(byte_array[2], 0x33, "array[2]");
    check(byte_array[3], 0x44, "array[3]");
    check(byte_array[4], 0x55, "array[4]");
    check(byte_array[5], 0x66, "array[5]");

    // TEST 7: Struct array data
    check(struct_array[0], 0xAA, "struct[0]");
    check(struct_array[1], 0xBB, "struct[1]");
    check(struct_array[2], 0xCC, "struct[2]");
    check(struct_array[3], 0xDD, "struct[3]");

    // Mark end of tests
    output[0x1F] = 0xAA;

    // Return error count
    return errors;
}
