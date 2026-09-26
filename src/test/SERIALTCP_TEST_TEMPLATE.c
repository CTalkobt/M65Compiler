/*
 * MEGA65 Test Template for serialtcp Output
 *
 * This is a template showing the correct format for tests
 * that will be validated via the serialtcp protocol.
 *
 * Key requirements:
 *   1. Use printf() for ALL output
 *   2. End with "RESULT:" line (completion marker)
 *   3. Format output for pattern matching
 *   4. Include [PASS]/[FAIL] assertions
 *   5. No memory writes needed
 */

#include <stdio.h>
#include <string.h>

/* Example: Global arrays to test */
unsigned char g_test_array[4] = {0xAA, 0xBB, 0xCC, 0xDD};
unsigned int g_int_array[3] = {100, 200, 300};

int main(void) {
    unsigned char i;
    int pass_count = 0;
    int fail_count = 0;

    /* ================================================================
     * TEST HEADER - Always start with header
     * ================================================================ */
    printf("\n");
    printf("=========================================\n");
    printf("MEGA65 Test: Example Array Test\n");
    printf("=========================================\n");
    printf("\n");


    /* ================================================================
     * TEST 1: Validate char array initialization
     * ================================================================ */
    printf("TEST 1: Char Array Initialization\n");
    printf("  Expected: [AA BB CC DD]\n");
    printf("  Actual:   [");
    for (i = 0; i < 4; i++) {
        printf("%02X", g_test_array[i]);
        if (i < 3) printf(" ");
    }
    printf("]\n");

    /* Verify values match expected */
    if (g_test_array[0] == 0xAA &&
        g_test_array[1] == 0xBB &&
        g_test_array[2] == 0xCC &&
        g_test_array[3] == 0xDD) {
        printf("  ✓ [PASS] Char array correct\n");
        pass_count++;
    } else {
        printf("  ✗ [FAIL] Char array mismatch\n");
        fail_count++;
    }
    printf("\n");


    /* ================================================================
     * TEST 2: Validate int array initialization
     * ================================================================ */
    printf("TEST 2: Int Array Initialization\n");
    printf("  Expected: [64 C8 2C]\n");  /* 100, 200, 300 in hex */
    printf("  Actual:   [");
    for (i = 0; i < 3; i++) {
        printf("%02X", g_int_array[i] & 0xFF);  /* Low byte only */
        if (i < 2) printf(" ");
    }
    printf("]\n");

    if (g_int_array[0] == 100 &&
        g_int_array[1] == 200 &&
        g_int_array[2] == 300) {
        printf("  ✓ [PASS] Int array correct\n");
        pass_count++;
    } else {
        printf("  ✗ [FAIL] Int array mismatch\n");
        fail_count++;
    }
    printf("\n");


    /* ================================================================
     * TEST 3: Local array initialization
     * ================================================================ */
    printf("TEST 3: Local Stack Array\n");
    unsigned char local_array[3] = {0x11, 0x22, 0x33};
    printf("  Expected: [11 22 33]\n");
    printf("  Actual:   [");
    for (i = 0; i < 3; i++) {
        printf("%02X", local_array[i]);
        if (i < 2) printf(" ");
    }
    printf("]\n");

    if (local_array[0] == 0x11 &&
        local_array[1] == 0x22 &&
        local_array[2] == 0x33) {
        printf("  ✓ [PASS] Local array correct\n");
        pass_count++;
    } else {
        printf("  ✗ [FAIL] Local array mismatch\n");
        fail_count++;
    }
    printf("\n");


    /* ================================================================
     * SUMMARY - Required format
     * ================================================================ */
    printf("=========================================\n");
    printf("SUMMARY\n");
    printf("=========================================\n");
    printf("\n");
    printf("  Tests passed: %d\n", pass_count);
    printf("  Tests failed: %d\n", fail_count);
    printf("\n");

    /* Final result line - REQUIRED for completion detection */
    if (fail_count == 0 && pass_count > 0) {
        printf("RESULT: ALL TESTS PASSED\n");
    } else {
        printf("RESULT: SOME TESTS FAILED\n");
    }

    printf("=========================================\n");
    printf("\n");

    /* Keep program running (required for xemu) */
    while (1)
        ;

    return 0;
}


/*
 * ====================================================================
 * GUIDELINES FOR serialtcp TEST FORMAT
 * ====================================================================
 *
 * 1. HEADER
 *    - Start with printf header
 *    - Include test name and separator lines
 *    - Makes output readable
 *
 * 2. EACH TEST
 *    - Print test name
 *    - Print expected values
 *    - Print actual values
 *    - Verify and print result
 *    - Include [PASS] or [FAIL] marker
 *
 * 3. FORMAT FOR ARRAYS
 *    - Use [HH HH HH] format (hex, space separated)
 *    - Print one complete array per line
 *    - Example: [AA BB CC DD]
 *    - Listener will pattern match these exact strings
 *
 * 4. ASSERTIONS
 *    - Every test should check one thing
 *    - Print "[PASS]" on success
 *    - Print "[FAIL]" on failure
 *    - Listener counts these automatically
 *
 * 5. FINAL RESULT
 *    - MUST end with "RESULT:" line
 *    - Text after "RESULT:" is the message
 *    - Examples:
 *      - "RESULT: ALL TESTS PASSED"
 *      - "RESULT: SOME TESTS FAILED"
 *      - "RESULT: 10/10 assertions passed"
 *    - This line signals completion to listener
 *
 * 6. PROGRAM LOOP
 *    - Must include "while(1);" at end
 *    - Keeps emulation running until killed
 *    - Allows listener to finish reading output
 *
 * 7. WHAT NOT TO DO
 *    - Don't use memory writes (no longer needed!)
 *    - Don't use completion markers at memory locations
 *    - Don't use timeout-based exit
 *    - Don't rely on emulator state inspection
 *
 * ====================================================================
 * VALIDATION PATTERN MATCHING
 * ====================================================================
 *
 * The Python listener looks for exact patterns in output:
 *
 *   '[AA BB CC DD]'   - Matches char array [0xAA, 0xBB, 0xCC, 0xDD]
 *   '[64 C8 2C]'      - Matches values 100, 200, 300 (low bytes)
 *   '[PASS]'          - Counts successful assertions
 *   '[FAIL]'          - Counts failed assertions
 *   'RESULT:'         - Signals test completion
 *
 * The listener:
 *   1. Captures all output lines from printf
 *   2. Searches for expected array patterns
 *   3. Counts [PASS] and [FAIL] markers
 *   4. Looks for "RESULT:" to detect completion
 *   5. Validates that all patterns are found
 *   6. Reports pass/fail based on matches
 *
 * ====================================================================
 */
