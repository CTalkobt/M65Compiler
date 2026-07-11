// Validation for addr_elem.16 and struct_elem.16 simulated opcodes
// This test validates that the assembly functions work correctly

#include <string.h>

volatile char *output = (char *)0x4000;

// Mirror of test array from assembly
unsigned char test_array[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

// Global data for stride tests
unsigned char global_data[16] = {
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x12, 0x34,
    0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22
};

void main() {
    volatile unsigned char *result = output;
    int errors = 0;
    int test_num = 0;

    // TEST 1: struct_elem.16 with global constant base, constant offset
    // Expected: test_array[1] = 0x33
    test_num++;
    if (result[0] != 0x33) {
        output[0x20 + test_num] = test_num;  // Mark error
        errors++;
    }

    // TEST 2: struct_elem.16 with ZP base, constant offset
    // Expected: test_array[0] + 1 = 0x22 (offset into second byte of first element)
    test_num++;
    if (result[1] != 0x22) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 3: struct_elem.16 with register base (.AX), constant offset
    // Expected: 0x44
    test_num++;
    if (result[2] != 0x44) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 4: struct_elem.16 with zero offset
    // Expected: test_array[0] = 0x11
    test_num++;
    if (result[3] != 0x11) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 5: struct_elem.16 with larger offset
    // Expected: test_array[2] = 0x55
    test_num++;
    if (result[4] != 0x55) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 6: addr_elem.16 with constant index 2, stride=1
    // Expected: test_array[2] = 0x33
    test_num++;
    if (result[5] != 0x33) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 7: addr_elem.16 with register index 3, stride=1
    // Expected: test_array[3] = 0x44
    test_num++;
    if (result[6] != 0x44) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 8: addr_elem.16 with ZP index 1, stride=1
    // Expected: test_array[1] = 0x22
    test_num++;
    if (result[7] != 0x22) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 9: addr_elem.16 with stride=2
    // Index 1, stride 2: global_data + 2 = 0xCC
    test_num++;
    if (result[8] != 0xCC) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 10: addr_elem.16 with index=0 (edge case)
    // Expected: test_array[0] = 0x11
    test_num++;
    if (result[9] != 0x11) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 11: struct_elem.16 with ZP base and result
    // Expected: test_array[2] = 0x33
    test_num++;
    if (result[10] != 0x33) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // TEST 12: addr_elem.16 with stride=4
    // Index 1, stride 4: global_data + 4 = 0xEE
    test_num++;
    if (result[11] != 0xEE) {
        output[0x20 + test_num] = test_num;
        errors++;
    }

    // Return error count in A
    return errors;
}
