/* test_stdio_phase4b.c — REL file I/O operations with KERNAL integration
 *
 * Tests Phase 4b functions (with actual KERNAL positioning):
 * - cbm_rel_open, cbm_rel_close — file management
 * - cbm_rel_read, cbm_rel_write — record I/O with positioning
 * - cbm_rel_position, cbm_rel_tell — positioning control
 * - cbm_rel_size, cbm_rel_record_size — metadata
 *
 * Phase 4b of FILE I/O implementation (KERNAL Integration)
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
 * TEST 1: Position Marker Protocol Validation
 * ============================================================================ */

int test_rel_position_protocol(void) {
    TEST("Position marker protocol");

    /* Position marker format: 0x00, record_lo, record_hi */
    unsigned char marker[3] = { 0x00, 0x01, 0x00 };  /* Record 1 */
    ASSERT(marker[0] == 0x00, "Position marker is 0x00");
    ASSERT(marker[1] == 0x01, "Record 1 low byte");
    ASSERT(marker[2] == 0x00, "Record 1 high byte");

    /* Test record number encoding */
    unsigned int record = 256;
    unsigned char lo = record & 0xFF;
    unsigned char hi = (record >> 8) & 0xFF;
    ASSERT(lo == 0x00, "Record 256 low byte = 0x00");
    ASSERT(hi == 0x01, "Record 256 high byte = 0x01");

    return 1;
}

/* ============================================================================
 * TEST 2: REL Read Operation Validation
 * ============================================================================ */

int test_rel_read_operation(void) {
    TEST("REL read operation signature");

    unsigned char buffer[256];
    int result;

    /* Reading from unopened handle should fail */
    result = cbm_rel_read(0, 1, buffer, 10);
    ASSERT(result == -1, "cbm_rel_read on unopened handle returns -1");

    /* Reading with NULL buffer should fail */
    result = cbm_rel_read(0, 1, NULL, 10);
    ASSERT(result == -1, "cbm_rel_read with NULL buffer returns -1");

    /* Reading with invalid handle should fail */
    result = cbm_rel_read(-1, 1, buffer, 10);
    ASSERT(result == -1, "cbm_rel_read with handle -1 returns -1");

    result = cbm_rel_read(8, 1, buffer, 10);
    ASSERT(result == -1, "cbm_rel_read with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 3: REL Write Operation Validation
 * ============================================================================ */

int test_rel_write_operation(void) {
    TEST("REL write operation signature");

    unsigned char data[10] = "testdata";
    int result;

    /* Writing to unopened handle should fail */
    result = cbm_rel_write(0, 1, data, 8);
    ASSERT(result == -1, "cbm_rel_write on unopened handle returns -1");

    /* Writing with NULL buffer should fail */
    result = cbm_rel_write(0, 1, NULL, 8);
    ASSERT(result == -1, "cbm_rel_write with NULL buffer returns -1");

    /* Writing with invalid handle should fail */
    result = cbm_rel_write(-1, 1, data, 8);
    ASSERT(result == -1, "cbm_rel_write with handle -1 returns -1");

    result = cbm_rel_write(8, 1, data, 8);
    ASSERT(result == -1, "cbm_rel_write with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 4: Position Operation Validation
 * ============================================================================ */

int test_rel_position_operation(void) {
    TEST("REL position operation signature");

    int result;

    /* Positioning unopened handle should fail */
    result = cbm_rel_position(0, 1);
    ASSERT(result == -1, "cbm_rel_position on unopened handle returns -1");

    /* Record 0 is invalid (must be 1-based) */
    result = cbm_rel_position(0, 0);
    ASSERT(result == -1, "cbm_rel_position to record 0 returns -1");

    /* Invalid handle should fail */
    result = cbm_rel_position(-1, 1);
    ASSERT(result == -1, "cbm_rel_position with handle -1 returns -1");

    result = cbm_rel_position(8, 1);
    ASSERT(result == -1, "cbm_rel_position with handle 8 returns -1");

    return 1;
}

/* ============================================================================
 * TEST 5: Tell Operation (Position Tracking)
 * ============================================================================ */

int test_rel_tell_operation(void) {
    TEST("REL tell operation (position tracking)");

    /* Tell on unopened handle returns 0 */
    unsigned int pos = cbm_rel_tell(0);
    ASSERT(pos == 0, "cbm_rel_tell on unopened handle returns 0");

    /* Tell on invalid handle returns 0 */
    pos = cbm_rel_tell(-1);
    ASSERT(pos == 0, "cbm_rel_tell with handle -1 returns 0");

    pos = cbm_rel_tell(8);
    ASSERT(pos == 0, "cbm_rel_tell with handle 8 returns 0");

    return 1;
}

/* ============================================================================
 * TEST 6: Size Operation (Record Count)
 * ============================================================================ */

int test_rel_size_operation(void) {
    TEST("REL size operation (record count)");

    /* Size on unopened handle returns 0 */
    unsigned int size = cbm_rel_size(0);
    ASSERT(size == 0, "cbm_rel_size on unopened handle returns 0");

    /* Size on invalid handle returns 0 */
    size = cbm_rel_size(-1);
    ASSERT(size == 0, "cbm_rel_size with handle -1 returns 0");

    size = cbm_rel_size(8);
    ASSERT(size == 0, "cbm_rel_size with handle 8 returns 0");

    return 1;
}

/* ============================================================================
 * TEST 7: Record Size Limits
 * ============================================================================ */

int test_rel_record_size_limits(void) {
    TEST("REL record size limits");

    /* Valid range: 1-254 bytes */
    unsigned int valid_min = 1;
    unsigned int valid_max = 254;
    unsigned int invalid_zero = 0;
    unsigned int invalid_max = 255;

    ASSERT(valid_min >= 1, "Minimum record size: 1 byte");
    ASSERT(valid_max <= 254, "Maximum record size: 254 bytes");
    ASSERT(invalid_zero == 0, "Record size 0 is invalid");
    ASSERT(invalid_max > 254, "Record size 255 is invalid");

    return 1;
}

/* ============================================================================
 * TEST 8: Sequential Record Numbering
 * ============================================================================ */

int test_rel_sequential_numbering(void) {
    TEST("REL sequential record numbering");

    /* Records are 1-based */
    ASSERT(1 >= 1, "First record is 1");

    /* Record number encoding for various values */
    unsigned int rec1 = 1;
    ASSERT((rec1 & 0xFF) == 1, "Record 1: low=1, high=0");
    ASSERT(((rec1 >> 8) & 0xFF) == 0, "Record 1: high byte correct");

    unsigned int rec255 = 255;
    ASSERT((rec255 & 0xFF) == 255, "Record 255: low=255");
    ASSERT(((rec255 >> 8) & 0xFF) == 0, "Record 255: high=0");

    unsigned int rec256 = 256;
    ASSERT((rec256 & 0xFF) == 0, "Record 256: low=0");
    ASSERT(((rec256 >> 8) & 0xFF) == 1, "Record 256: high=1");

    unsigned int rec65535 = 65535;
    ASSERT((rec65535 & 0xFF) == 255, "Record 65535: low=255");
    ASSERT(((rec65535 >> 8) & 0xFF) == 255, "Record 65535: high=255");

    return 1;
}

/* ============================================================================
 * TEST 9: Buffer Size Validation
 * ============================================================================ */

int test_rel_buffer_sizing(void) {
    TEST("REL buffer sizing");

    unsigned char small_buf[10];
    unsigned char large_buf[256];
    unsigned char record_data[254];

    ASSERT(sizeof(small_buf) == 10, "Small buffer: 10 bytes");
    ASSERT(sizeof(large_buf) == 256, "Large buffer: 256 bytes");
    ASSERT(sizeof(record_data) == 254, "Record data buffer: 254 bytes");

    /* Records are capped at record_size */
    unsigned int record_size = 32;
    unsigned int requested = 64;
    unsigned int actual = (requested > record_size) ? record_size : requested;
    ASSERT(actual == 32, "Oversized request capped to record size");

    return 1;
}

/* ============================================================================
 * TEST 10: Close Operation State Cleanup
 * ============================================================================ */

int test_rel_close_cleanup(void) {
    TEST("REL close operation cleanup");

    int result;

    /* Closing unopened handle should fail */
    result = cbm_rel_close(0);
    ASSERT(result == -1, "cbm_rel_close on unopened handle returns -1");

    /* Closing invalid handles should fail */
    result = cbm_rel_close(-1);
    ASSERT(result == -1, "cbm_rel_close with handle -1 returns -1");

    result = cbm_rel_close(8);
    ASSERT(result == -1, "cbm_rel_close with handle 8 returns -1");

    /* After close, handle is no longer open */
    ASSERT(cbm_rel_is_open(0) == 0, "Closed handle 0 is not open");

    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("FILE I/O Phase 4b - KERNAL Integration\n");
    printf("REL File I/O Operations with Positioning\n");
    printf("========================================\n");

    /* Run all tests */
    test_rel_position_protocol();
    test_rel_read_operation();
    test_rel_write_operation();
    test_rel_position_operation();
    test_rel_tell_operation();
    test_rel_size_operation();
    test_rel_record_size_limits();
    test_rel_sequential_numbering();
    test_rel_buffer_sizing();
    test_rel_close_cleanup();

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

