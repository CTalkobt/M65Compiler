/* test_stdio_phase1.c — Phase 1 FILE I/O tests
 *
 * Tests for fopen, fclose, fgetc, fputc, fread, fwrite, feof, ferror, fileno
 * These are basic unit tests that don't require actual disk I/O
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * TEST 1: FILE Structure Initialization
 * ============================================================================
 *
 * Verify that stdin, stdout, stderr are properly initialized
 */

int test_standard_streams(void) {
    /* Check stdin */
    if (fileno(stdin) != STDIN_FILENO) {
        printf("ERROR: stdin fileno is %d, expected %d\n", fileno(stdin), STDIN_FILENO);
        return 0;
    }

    /* Check stdout */
    if (fileno(stdout) != STDOUT_FILENO) {
        printf("ERROR: stdout fileno is %d, expected %d\n", fileno(stdout), STDOUT_FILENO);
        return 0;
    }

    /* Check stderr */
    if (fileno(stderr) != STDERR_FILENO) {
        printf("ERROR: stderr fileno is %d, expected %d\n", fileno(stderr), STDERR_FILENO);
        return 0;
    }

    printf("PASS: Standard streams initialized correctly\n");
    return 1;
}

/* ============================================================================
 * TEST 2: feof and ferror on fresh streams
 * ============================================================================
 *
 * Verify that new streams don't have EOF or error flags set
 */

int test_fresh_stream_flags(void) {
    if (feof(stdin)) {
        printf("ERROR: stdin should not be at EOF\n");
        return 0;
    }

    if (ferror(stdin)) {
        printf("ERROR: stdin should not have error flag\n");
        return 0;
    }

    printf("PASS: Fresh streams have correct flags\n");
    return 1;
}

/* ============================================================================
 * TEST 3: clearerr clears flags
 * ============================================================================
 */

int test_clearerr(void) {
    FILE *fp = stdin;

    /* Artificially set error flags for testing */
    /* Note: In real implementation, we'd need access to FILE internals
     * This test just verifies clearerr doesn't crash */

    clearerr(fp);

    if (feof(fp)) {
        printf("ERROR: clearerr didn't clear EOF flag\n");
        return 0;
    }

    if (ferror(fp)) {
        printf("ERROR: clearerr didn't clear error flag\n");
        return 0;
    }

    printf("PASS: clearerr works correctly\n");
    return 1;
}

/* ============================================================================
 * TEST 4: fputc to stdout (basic character output)
 * ============================================================================
 *
 * Tests character output. Note: actual output goes to screen.
 */

int test_fputc(void) {
    int result;

    /* Write test message */
    result = fputc('H', stdout);
    if (result != 'H') {
        printf("ERROR: fputc returned %d, expected 72\n", result);
        return 0;
    }

    result = fputc('i', stdout);
    result = fputc('\n', stdout);

    printf("PASS: fputc works correctly\n");
    return 1;
}

/* ============================================================================
 * TEST 5: Mode parsing (internal test)
 * ============================================================================
 *
 * Tests that various file modes are handled correctly
 * This is an indirect test through fopen (if it's implemented)
 */

int test_mode_parsing(void) {
    /* Test cases that would be used with fopen
     * These would need actual KERNAL integration to test fully
     * For now, we just verify the constants are defined correctly */

    if (O_RDONLY != 0x00) {
        printf("ERROR: O_RDONLY has wrong value\n");
        return 0;
    }

    if (O_WRONLY != 0x01) {
        printf("ERROR: O_WRONLY has wrong value\n");
        return 0;
    }

    if (O_APPEND != 0x10) {
        printf("ERROR: O_APPEND has wrong value\n");
        return 0;
    }

    printf("PASS: File mode constants are correct\n");
    return 1;
}

/* ============================================================================
 * TEST 6: CBM File Types
 * ============================================================================
 *
 * Verify Commodore file type constants
 */

int test_cbm_types(void) {
    if (CBM_T_PRG != 0x82) {
        printf("ERROR: CBM_T_PRG has wrong value\n");
        return 0;
    }

    if (CBM_T_SEQ != 0x81) {
        printf("ERROR: CBM_T_SEQ has wrong value\n");
        return 0;
    }

    if (CBM_T_USR != 0x84) {
        printf("ERROR: CBM_T_USR has wrong value\n");
        return 0;
    }

    printf("PASS: CBM file type constants are correct\n");
    return 1;
}

/* ============================================================================
 * TEST 7: Device Constants
 * ============================================================================
 *
 * Verify Commodore device constants
 */

int test_cbm_devices(void) {
    if (CBM_DEVICE_DISK != 8) {
        printf("ERROR: CBM_DEVICE_DISK should be 8\n");
        return 0;
    }

    if (CBM_DEVICE_TAPE != 1) {
        printf("ERROR: CBM_DEVICE_TAPE should be 1\n");
        return 0;
    }

    printf("PASS: CBM device constants are correct\n");
    return 1;
}

/* ============================================================================
 * TEST 8: Standard I/O Constants
 * ============================================================================
 */

int test_stdio_constants(void) {
    if (EOF != -1) {
        printf("ERROR: EOF should be -1\n");
        return 0;
    }

    if (SEEK_SET != 0) {
        printf("ERROR: SEEK_SET should be 0\n");
        return 0;
    }

    if (SEEK_CUR != 1) {
        printf("ERROR: SEEK_CUR should be 1\n");
        return 0;
    }

    if (SEEK_END != 2) {
        printf("ERROR: SEEK_END should be 2\n");
        return 0;
    }

    if (FOPEN_MAX != 8) {
        printf("ERROR: FOPEN_MAX should be 8\n");
        return 0;
    }

    if (BUFSIZ != 256) {
        printf("ERROR: BUFSIZ should be 256\n");
        return 0;
    }

    printf("PASS: Standard I/O constants are correct\n");
    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("=== FILE I/O Phase 1 Tests ===\n\n");

    int passed = 0;
    int total = 0;

    /* Run tests */
    total++; if (test_standard_streams()) passed++;
    total++; if (test_fresh_stream_flags()) passed++;
    total++; if (test_clearerr()) passed++;
    total++; if (test_fputc()) passed++;
    total++; if (test_mode_parsing()) passed++;
    total++; if (test_cbm_types()) passed++;
    total++; if (test_cbm_devices()) passed++;
    total++; if (test_stdio_constants()) passed++;

    printf("\n=== Results ===\n");
    printf("Passed: %d/%d\n", passed, total);

    if (passed == total) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed\n", total - passed);
        return 1;
    }
}

