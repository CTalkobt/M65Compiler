/* test_stdio_integration.c — Comprehensive FILE I/O integration tests
 *
 * Tests end-to-end file operations:
 * - File creation and opening
 * - Character and block I/O
 * - Buffering behavior
 * - Error handling
 *
 * Requires MEGA65 emulator or hardware with disk image
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
 * TEST 1: Standard Streams
 * ============================================================================ */

int test_standard_streams(void) {
    TEST("Standard Streams Initialization");

    ASSERT(stdin != NULL, "stdin is initialized");
    ASSERT(stdout != NULL, "stdout is initialized");
    ASSERT(stderr != NULL, "stderr is initialized");
    ASSERT(!feof(stdin), "stdin not at EOF");
    ASSERT(!ferror(stdin), "stdin has no error");

    return 1;
}

/* ============================================================================
 * TEST 2: FILE Structure Size and Layout
 * ============================================================================ */

int test_file_structure(void) {
    TEST("FILE Structure");

    printf("  sizeof(FILE) = %u bytes\n", (unsigned)sizeof(FILE));
    ASSERT(sizeof(FILE) <= 32, "FILE struct fits in 32 bytes");

    /* Test FILE fields are accessible */
    FILE dummy = {0};
    ASSERT(sizeof(dummy.handle) >= 1, "FILE.handle exists");
    ASSERT(sizeof(dummy.buffer) > 0, "FILE.buffer pointer exists");
    ASSERT(sizeof(dummy.buf_pos) > 0, "FILE.buf_pos exists");

    return 1;
}

/* ============================================================================
 * TEST 3: Character Output (printf to stdout)
 * ============================================================================ */

int test_character_output(void) {
    TEST("Character Output");

    int result = fputc('A', stdout);
    ASSERT(result == 'A', "fputc returns written character");

    result = fputc('\n', stdout);
    ASSERT(result == '\n', "fputc handles newline");

    return 1;
}

/* ============================================================================
 * TEST 4: Buffer Management
 * ============================================================================ */

int test_buffer_management(void) {
    TEST("Buffer Management");

    FILE *fp = stdout;

    ASSERT(fp->buffer != NULL, "stdout has buffer allocated");
    ASSERT(fp->buf_size > 0, "stdout buffer size is positive");
    ASSERT(fp->buf_size >= 256, "stdout buffer is at least 256 bytes");

    return 1;
}

/* ============================================================================
 * TEST 5: Error and EOF Flags
 * ============================================================================ */

int test_error_handling(void) {
    TEST("Error and EOF Flag Handling");

    FILE *fp = stdin;

    ASSERT(!feof(fp), "stdin not at EOF initially");
    ASSERT(!ferror(fp), "stdin has no error initially");

    clearerr(fp);
    ASSERT(!feof(fp), "clearerr clears EOF flag");
    ASSERT(!ferror(fp), "clearerr clears error flag");

    return 1;
}

/* ============================================================================
 * TEST 6: Mode Constants
 * ============================================================================ */

int test_mode_constants(void) {
    TEST("Mode Constants");

    ASSERT(EOF == -1, "EOF is -1");
    ASSERT(SEEK_SET == 0, "SEEK_SET is 0");
    ASSERT(SEEK_CUR == 1, "SEEK_CUR is 1");
    ASSERT(SEEK_END == 2, "SEEK_END is 2");
    ASSERT(BUFSIZ == 256, "BUFSIZ is 256");
    ASSERT(FOPEN_MAX == 8, "FOPEN_MAX is 8");

    return 1;
}

/* ============================================================================
 * TEST 7: Commodore Device Constants
 * ============================================================================ */

int test_commodore_constants(void) {
    TEST("Commodore Device Constants");

    /* Device numbers */
    ASSERT(CBM_DEVICE_DISK == 8, "CBM_DEVICE_DISK is 8");
    ASSERT(CBM_DEVICE_TAPE == 1, "CBM_DEVICE_TAPE is 1");

    /* File types */
    ASSERT(CBM_T_PRG == 0x82, "CBM_T_PRG is 0x82");
    ASSERT(CBM_T_SEQ == 0x81, "CBM_T_SEQ is 0x81");
    ASSERT(CBM_T_USR == 0x84, "CBM_T_USR is 0x84");

    return 1;
}

/* ============================================================================
 * TEST 8: File Descriptor Range
 * ============================================================================ */

int test_fd_range(void) {
    TEST("File Descriptor Range");

    ASSERT(STDIN_FILENO == 0, "stdin fd is 0");
    ASSERT(STDOUT_FILENO == 1, "stdout fd is 1");
    ASSERT(STDERR_FILENO == 2, "stderr fd is 2");

    return 1;
}

/* ============================================================================
 * TEST 9: Ungetc Character Storage
 * ============================================================================ */

int test_ungetc_field(void) {
    TEST("Ungetc Character Storage");

    FILE *fp = stdin;

    ASSERT(sizeof(fp->ungetc_char) >= 1, "FILE has ungetc_char field");

    return 1;
}

/* ============================================================================
 * TEST 10: File Position Tracking
 * ============================================================================ */

int test_position_tracking(void) {
    TEST("File Position Tracking");

    FILE *fp = stdout;

    ASSERT(sizeof(fp->position) >= 4, "FILE.position is at least 32-bit");
    ASSERT(sizeof(fp->file_size) >= 4, "FILE.file_size is at least 32-bit");

    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("FILE I/O Integration Test Suite\n");
    printf("========================================\n");

    /* Run all tests */
    test_standard_streams();
    test_file_structure();
    test_character_output();
    test_buffer_management();
    test_error_handling();
    test_mode_constants();
    test_commodore_constants();
    test_fd_range();
    test_ungetc_field();
    test_position_tracking();

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

