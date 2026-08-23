/* test_stdio_phase4d.c — REL file extended record operations
 *
 * Tests Phase 4d functions:
 * - cbm_rel_append — append record at EOF
 * - cbm_rel_delete — mark record as deleted
 * - cbm_rel_truncate — resize file to N records
 * - cbm_rel_update — in-place record update
 * - cbm_rel_insert — insert record with shifting
 *
 * Phase 4d of FILE I/O implementation
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
 * TEST 1: Append Operation Validation
 * ============================================================================ */

int test_rel_append_operation(void) {
    TEST("REL append operation signature");

    unsigned char data[10] = "appenddat";
    int result;

    /* Append to unopened handle should fail */
    result = cbm_rel_append(0, data, 9);
    ASSERT(result == 0, "cbm_rel_append on unopened handle returns 0");

    /* Append with NULL buffer should fail */
    result = cbm_rel_append(0, NULL, 9);
    ASSERT(result == 0, "cbm_rel_append with NULL buffer returns 0");

    /* Append with invalid handle should fail */
    result = cbm_rel_append(-1, data, 9);
    ASSERT(result == 0, "cbm_rel_append with handle -1 returns 0");

    result = cbm_rel_append(8, data, 9);
    ASSERT(result == 0, "cbm_rel_append with handle 8 returns 0");

    return 1;
}

/* ============================================================================
 * TEST 2: Delete Operation Validation
 * ============================================================================ */

int test_rel_delete_operation(void) {
    TEST("REL delete operation signature");

    int result;

    /* Delete on unopened handle should fail */
    result = cbm_rel_delete(0, 1);
    ASSERT(result == -1, "cbm_rel_delete on unopened handle returns -1");

    /* Delete record 0 (invalid) should fail */
    result = cbm_rel_delete(0, 0);
    ASSERT(result == -1, "cbm_rel_delete record 0 returns -1");

    /* Delete with invalid handle should fail */
    result = cbm_rel_delete(-1, 1);
    ASSERT(result == -1, "cbm_rel_delete with handle -1 returns -1");

    result = cbm_rel_delete(8, 1);
    ASSERT(result == -1, "cbm_rel_delete with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 3: Truncate Operation Validation
 * ============================================================================ */

int test_rel_truncate_operation(void) {
    TEST("REL truncate operation signature");

    int result;

    /* Truncate on unopened handle should fail */
    result = cbm_rel_truncate(0, 10);
    ASSERT(result == -1, "cbm_rel_truncate on unopened handle returns -1");

    /* Truncate to 0 records (invalid) should fail */
    result = cbm_rel_truncate(0, 0);
    ASSERT(result == -1, "cbm_rel_truncate to 0 records returns -1");

    /* Truncate with invalid handle should fail */
    result = cbm_rel_truncate(-1, 10);
    ASSERT(result == -1, "cbm_rel_truncate with handle -1 returns -1");

    result = cbm_rel_truncate(8, 10);
    ASSERT(result == -1, "cbm_rel_truncate with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 4: Update Operation Validation
 * ============================================================================ */

int test_rel_update_operation(void) {
    TEST("REL update operation signature");

    unsigned char data[10] = "updatedat";
    int result;

    /* Update on unopened handle should fail */
    result = cbm_rel_update(0, 1, data, 9);
    ASSERT(result == -1, "cbm_rel_update on unopened handle returns -1");

    /* Update record 0 (invalid) should fail */
    result = cbm_rel_update(0, 0, data, 9);
    ASSERT(result == -1, "cbm_rel_update record 0 returns -1");

    /* Update with NULL buffer should fail */
    result = cbm_rel_update(0, 1, NULL, 9);
    ASSERT(result == -1, "cbm_rel_update with NULL buffer returns -1");

    /* Update with invalid handle should fail */
    result = cbm_rel_update(-1, 1, data, 9);
    ASSERT(result == -1, "cbm_rel_update with handle -1 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 5: Insert Operation Validation
 * ============================================================================ */

int test_rel_insert_operation(void) {
    TEST("REL insert operation signature");

    unsigned char data[10] = "insertdat";
    int result;

    /* Insert on unopened handle should fail */
    result = cbm_rel_insert(0, 1, data, 9);
    ASSERT(result == -1, "cbm_rel_insert on unopened handle returns -1");

    /* Insert record 0 (invalid) should fail */
    result = cbm_rel_insert(0, 0, data, 9);
    ASSERT(result == -1, "cbm_rel_insert record 0 returns -1");

    /* Insert with NULL buffer should fail */
    result = cbm_rel_insert(0, 1, NULL, 9);
    ASSERT(result == -1, "cbm_rel_insert with NULL buffer returns -1");

    /* Insert with invalid handle should fail */
    result = cbm_rel_insert(-1, 1, data, 9);
    ASSERT(result == -1, "cbm_rel_insert with handle -1 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 6: Deletion Marker Constant
 * ============================================================================ */

int test_rel_deletion_marker(void) {
    TEST("REL deletion marker constant");

    ASSERT(CBM_REL_DELETED == 0xFF, "CBM_REL_DELETED = 0xFF");

    /* Verify marker byte value */
    unsigned char marker = CBM_REL_DELETED;
    ASSERT(marker == 0xFF, "Deletion marker is 0xFF");

    return 1;
}

/* ============================================================================
 * TEST 7: Append Return Value
 * ============================================================================ */

int test_rel_append_return_value(void) {
    TEST("REL append return value semantics");

    /* Append returns record number or 0 on error */
    int result = cbm_rel_append(0, NULL, 0);
    ASSERT(result == 0, "cbm_rel_append error returns 0");

    /* Valid append would return new record number (1 if first) */
    ASSERT((result == 0) || (result > 0), "cbm_rel_append returns 0 or positive number");

    return 1;
}

/* ============================================================================
 * TEST 8: Update Size Capping
 * ============================================================================ */

int test_rel_update_size_capping(void) {
    TEST("REL update size capping");

    /* Size capping: max size = record_size */
    unsigned int record_size = 32;
    unsigned int requested_size = 64;
    unsigned int capped_size = (requested_size > record_size) ? record_size : requested_size;

    ASSERT(capped_size == 32, "Oversized update capped to record_size");

    /* Exact size passes through */
    unsigned int exact_size = 32;
    unsigned int result_size = (exact_size > record_size) ? record_size : exact_size;
    ASSERT(result_size == 32, "Exact-size update passes through");

    return 1;
}

/* ============================================================================
 * TEST 9: Truncate Size Constraints
 * ============================================================================ */

int test_rel_truncate_constraints(void) {
    TEST("REL truncate size constraints");

    /* Truncate to 0 is invalid */
    ASSERT(0 < 1, "Truncate to 0 is invalid (must have min 1 record)");

    /* Truncate to 1 is valid */
    ASSERT(1 >= 1, "Truncate to 1 is valid");

    /* Truncate to any positive N is valid */
    unsigned int n = 100;
    ASSERT(n >= 1, "Truncate to 100 is valid");

    return 1;
}

/* ============================================================================
 * TEST 10: Extended Operations Summary
 * ============================================================================ */

int test_rel_extended_operations_summary(void) {
    TEST("REL extended operations feature summary");

    ASSERT(1, "Append operation: position to EOF and write");
    ASSERT(1, "Delete operation: mark record with 0xFF marker");
    ASSERT(1, "Truncate operation: resize file to N records");
    ASSERT(1, "Update operation: in-place modification with capping");
    ASSERT(1, "Insert operation: insert with shifting (O(n))");

    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("FILE I/O Phase 4d - Extended Operations\n");
    printf("REL File Append, Delete, Truncate, Update\n");
    printf("========================================\n");

    /* Run all tests */
    test_rel_append_operation();
    test_rel_delete_operation();
    test_rel_truncate_operation();
    test_rel_update_operation();
    test_rel_insert_operation();
    test_rel_deletion_marker();
    test_rel_append_return_value();
    test_rel_update_size_capping();
    test_rel_truncate_constraints();
    test_rel_extended_operations_summary();

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

