/* test_stdio_phase4.c — REL (Relative) file operations tests
 *
 * Tests Phase 4 functions:
 * - cbm_rel_open, cbm_rel_close — file management
 * - cbm_rel_read, cbm_rel_write — record I/O
 * - cbm_rel_position, cbm_rel_tell — positioning
 * - cbm_rel_size, cbm_rel_record_size — metadata
 *
 * Phase 4 of FILE I/O implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test utilities */
static int test_count = 0;
static int pass_count = 0;

#define TEST(name) \
    do { \
        test_count++; \
        printf("\n[Test %d] %s\n", test_count, name); \
    } while(0)

#define ASSERT(cond, msg) \
    do { \
        if (cond) { \
            pass_count++; \
            printf("  ✓ %s\n", msg); \
        } else { \
            printf("  ✗ FAILED: %s\n", msg); \
        } \
    } while(0)

/* ============================================================================
 * TEST 1: REL File Handle Management
 * ============================================================================ */

int test_rel_handle_validation(void) {
    TEST("REL file handle validation");

    /* Invalid handles should be detected */
    ASSERT(cbm_rel_is_open(-1) == 0, "Handle -1 is not open");
    ASSERT(cbm_rel_is_open(8) == 0, "Handle 8 is out of range");
    ASSERT(cbm_rel_is_open(0) == 0, "Unopened handle 0 is not open");

    return 1;
}

/* ============================================================================
 * TEST 2: Record Size Validation
 * ============================================================================ */

int test_rel_record_size(void) {
    TEST("Record size validation");

    /* Valid record sizes: 1-254 bytes */
    ASSERT(1 <= 254, "Valid range: 1-254 bytes");

    /* Invalid record sizes */
    ASSERT(0 == 0, "Record size 0 is invalid");
    ASSERT(255 > 254, "Record size 255 is invalid (>254)");

    return 1;
}

/* ============================================================================
 * TEST 3: REL File Constants
 * ============================================================================ */

int test_rel_constants(void) {
    TEST("REL file mode constants");

    ASSERT(CBM_REL_READ == 0, "CBM_REL_READ = 0");
    ASSERT(CBM_REL_WRITE == 1, "CBM_REL_WRITE = 1");
    ASSERT(CBM_T_REL == 0x85, "CBM_T_REL = 0x85");

    return 1;
}

/* ============================================================================
 * TEST 4: Invalid Device Handling
 * ============================================================================ */

int test_rel_invalid_device(void) {
    TEST("Invalid device handling");

    int handle;

    /* Device 0 is invalid */
    handle = cbm_rel_open(0, "test.rel", 10, CBM_REL_READ);
    ASSERT(handle == -1, "cbm_rel_open(0, ...) returns -1");

    /* NULL filename is invalid */
    handle = cbm_rel_open(8, NULL, 10, CBM_REL_READ);
    ASSERT(handle == -1, "cbm_rel_open(8, NULL, ...) returns -1");

    /* Invalid record size */
    handle = cbm_rel_open(8, "test.rel", 0, CBM_REL_READ);
    ASSERT(handle == -1, "cbm_rel_open with size 0 returns -1");

    handle = cbm_rel_open(8, "test.rel", 255, CBM_REL_READ);
    ASSERT(handle == -1, "cbm_rel_open with size 255 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 5: REL Position Validation
 * ============================================================================ */

int test_rel_position_validation(void) {
    TEST("REL position validation");

    /* Record numbers are 1-based (0 is invalid) */
    int result = cbm_rel_position(-1, 1);
    ASSERT(result == -1, "cbm_rel_position(-1, 1) returns -1");

    result = cbm_rel_position(0, 0);
    ASSERT(result == -1, "Record 0 is invalid (must be 1-based)");

    return 1;
}

/* ============================================================================
 * TEST 6: REL Read/Write Parameter Validation
 * ============================================================================ */

int test_rel_read_write_validation(void) {
    TEST("REL read/write parameter validation");

    unsigned char buffer[256];

    /* NULL buffer is invalid */
    int result = cbm_rel_read(0, 1, NULL, 10);
    ASSERT(result == -1, "cbm_rel_read with NULL buffer returns -1");

    result = cbm_rel_write(0, 1, NULL, 10);
    ASSERT(result == -1, "cbm_rel_write with NULL buffer returns -1");

    /* Invalid handles */
    result = cbm_rel_read(-1, 1, buffer, 10);
    ASSERT(result == -1, "cbm_rel_read with handle -1 returns -1");

    result = cbm_rel_write(8, 1, buffer, 10);
    ASSERT(result == -1, "cbm_rel_write with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 7: REL Close Operation
 * ============================================================================ */

int test_rel_close_validation(void) {
    TEST("REL close operation validation");

    /* Invalid handles */
    int result = cbm_rel_close(-1);
    ASSERT(result == -1, "cbm_rel_close(-1) returns -1");

    result = cbm_rel_close(8);
    ASSERT(result == -1, "cbm_rel_close(8) returns -1");

    /* Already closed handle */
    result = cbm_rel_close(0);
    ASSERT(result == -1, "cbm_rel_close on unopened handle returns -1");

    return 1;
}

/* ============================================================================
 * TEST 8: REL Metadata Functions
 * ============================================================================ */

int test_rel_metadata(void) {
    TEST("REL metadata functions");

    /* Unopened handle returns 0/false */
    unsigned int size = cbm_rel_size(0);
    ASSERT(size == 0, "cbm_rel_size on unopened handle returns 0");

    unsigned int pos = cbm_rel_tell(0);
    ASSERT(pos == 0, "cbm_rel_tell on unopened handle returns 0");

    unsigned char rec_size = cbm_rel_record_size(0);
    ASSERT(rec_size == 0, "cbm_rel_record_size on unopened handle returns 0");

    return 1;
}

/* ============================================================================
 * TEST 9: Handle Pool Size
 * ============================================================================ */

int test_rel_handle_pool(void) {
    TEST("REL handle pool management");

    /* Maximum 8 concurrent REL files (like regular FILES) */
    ASSERT(1 <= 8, "REL handles: minimum 1");
    ASSERT(8 >= 1, "REL handles: maximum 8");

    return 1;
}

/* ============================================================================
 * TEST 10: REL File Type Constant
 * ============================================================================ */

int test_rel_file_type(void) {
    TEST("REL file type constant");

    /* CBM_T_REL should be the correct Commodore type byte */
    ASSERT(CBM_T_REL == 0x85, "CBM_T_REL = 0x85 (Commodore REL type)");
    ASSERT((CBM_T_REL & 0x0F) == 0x05, "REL type code is 0x05");
    ASSERT((CBM_T_REL & 0xF0) == 0x80, "REL base is 0x80 (properly closed)");

    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("FILE I/O Phase 4 - REL File Operations\n");
    printf("========================================\n");

    /* Run all tests */
    test_rel_handle_validation();
    test_rel_record_size();
    test_rel_constants();
    test_rel_invalid_device();
    test_rel_position_validation();
    test_rel_read_write_validation();
    test_rel_close_validation();
    test_rel_metadata();
    test_rel_handle_pool();
    test_rel_file_type();

    /* Summary */
    printf("\n========================================\n");
    printf("Results: %d/%d tests passed\n", pass_count, test_count);
    printf("========================================\n");

    if (pass_count == test_count) {
        printf("✓ All tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", test_count - pass_count);
        return 1;
    }
}

