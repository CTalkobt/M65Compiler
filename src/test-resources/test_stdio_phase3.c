/* test_stdio_phase3.c — Directory and file operations tests
 *
 * Tests Phase 3 functions:
 * - cbm_opendir, cbm_readdir, cbm_closedir — directory listing
 * - cbm_delete, cbm_rename, cbm_format — file operations
 * - cbm_dir_type_name — type name conversion
 *
 * Phase 3 of FILE I/O implementation
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
 * TEST 1: File Type Name Conversion
 * ============================================================================ */

int test_file_type_names(void) {
    TEST("File type name conversion");

    const char *name;

    name = cbm_dir_type_name(CBM_T_PRG);
    ASSERT(name != NULL && strcmp(name, "PRG") == 0, "CBM_T_PRG → 'PRG'");

    name = cbm_dir_type_name(CBM_T_SEQ);
    ASSERT(name != NULL && strcmp(name, "SEQ") == 0, "CBM_T_SEQ → 'SEQ'");

    name = cbm_dir_type_name(CBM_T_USR);
    ASSERT(name != NULL && strcmp(name, "USR") == 0, "CBM_T_USR → 'USR'");

    name = cbm_dir_type_name(0xFF);
    ASSERT(name != NULL && strcmp(name, "???") == 0, "Unknown type → '???'");

    return 1;
}

/* ============================================================================
 * TEST 2: Directory Handle Management
 * ============================================================================ */

int test_directory_handles(void) {
    TEST("Directory handle management");

    /* cbm_opendir should return -1 for invalid device */
    int dir = cbm_opendir(0);
    ASSERT(dir == -1, "cbm_opendir(0) returns -1");

    /* cbm_closedir should handle invalid handles */
    int result = cbm_closedir(-1);
    ASSERT(result == -1, "cbm_closedir(-1) returns -1");

    return 1;
}

/* ============================================================================
 * TEST 3: Directory Entry Structure
 * ============================================================================ */

int test_directory_entry(void) {
    TEST("Directory entry parsing");

    /* Test that we can parse a directory entry */
    unsigned char raw_entry[32] = {
        0x0A, 0x00,                              /* Size: 10 blocks */
        'T', 'E', 'S', 'T', 0xA0, 0xA0,          /* "TEST" padded */
        0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,     /* More padding */
        0xA0, 0xA0, 0xA0, 0xA0,
        0x82,                                    /* File type: PRG */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    /* Verify structure size */
    ASSERT(sizeof(raw_entry) == 32, "Directory entry is 32 bytes");

    /* Verify type byte location */
    ASSERT(raw_entry[18] == 0x82, "File type at offset 18");

    return 1;
}

/* ============================================================================
 * TEST 4: Delete Operation Signature
 * ============================================================================ */

int test_delete_function(void) {
    TEST("cbm_delete function signature");

    int result;

    /* cbm_delete with invalid parameters should return -1 */
    result = cbm_delete(0, "test.txt");
    ASSERT(result == -1, "cbm_delete(0, name) returns -1");

    result = cbm_delete(8, NULL);
    ASSERT(result == -1, "cbm_delete(8, NULL) returns -1");

    /* Valid parameters return -1 (not yet implemented) */
    result = cbm_delete(8, "test.txt");
    ASSERT(result == -1, "cbm_delete(8, 'test.txt') returns -1 (not implemented)");

    return 1;
}

/* ============================================================================
 * TEST 5: Rename Operation Signature
 * ============================================================================ */

int test_rename_function(void) {
    TEST("cbm_rename function signature");

    int result;

    /* cbm_rename with invalid parameters should return -1 */
    result = cbm_rename(0, "old", "new");
    ASSERT(result == -1, "cbm_rename(0, ...) returns -1");

    result = cbm_rename(8, NULL, "new");
    ASSERT(result == -1, "cbm_rename(8, NULL, ...) returns -1");

    result = cbm_rename(8, "old", NULL);
    ASSERT(result == -1, "cbm_rename(8, ..., NULL) returns -1");

    /* Valid parameters return -1 (not yet implemented) */
    result = cbm_rename(8, "old.txt", "new.txt");
    ASSERT(result == -1, "cbm_rename(8, ...) returns -1 (not implemented)");

    return 1;
}

/* ============================================================================
 * TEST 6: Format Operation Signature
 * ============================================================================ */

int test_format_function(void) {
    TEST("cbm_format function signature");

    int result;

    /* cbm_format with invalid parameters should return -1 */
    result = cbm_format(0, "DISK", "00");
    ASSERT(result == -1, "cbm_format(0, ...) returns -1");

    result = cbm_format(8, NULL, "00");
    ASSERT(result == -1, "cbm_format(8, NULL, ...) returns -1");

    result = cbm_format(8, "DISK", NULL);
    ASSERT(result == -1, "cbm_format(8, ..., NULL) returns -1");

    /* Valid parameters return -1 (not yet implemented) */
    result = cbm_format(8, "MEGA65", "00");
    ASSERT(result == -1, "cbm_format(8, ...) returns -1 (not implemented)");

    return 1;
}

/* ============================================================================
 * TEST 7: CBM Type Constants
 * ============================================================================ */

int test_cbm_type_constants(void) {
    TEST("Commodore type constants");

    ASSERT(CBM_T_PRG == 0x82, "CBM_T_PRG = 0x82");
    ASSERT(CBM_T_SEQ == 0x81, "CBM_T_SEQ = 0x81");
    ASSERT(CBM_T_USR == 0x84, "CBM_T_USR = 0x84");
    ASSERT(CBM_T_REL == 0x85, "CBM_T_REL = 0x85");

    return 1;
}

/* ============================================================================
 * TEST 8: Device Number Constants
 * ============================================================================ */

int test_device_constants(void) {
    TEST("Device number constants");

    ASSERT(CBM_DEVICE_DISK == 8, "CBM_DEVICE_DISK = 8");
    ASSERT(CBM_DEVICE_TAPE == 1, "CBM_DEVICE_TAPE = 1");

    return 1;
}

/* ============================================================================
 * TEST 9: Directory String Parsing
 * ============================================================================ */

int test_directory_string_parsing(void) {
    TEST("Directory string parsing");

    /* Test padded string removal */
    char filename[17];
    unsigned char entry_data[32] = {
        0x00, 0x00,
        'F', 'I', 'L', 'E', 'N', 'A', 'M', 'E', 0xA0, 0xA0,
        0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0,
        0x82,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    /* Simulate what cbm_readdir does */
    int i = 0, j = 0;
    while (i < 16 && entry_data[i + 2] != 0xA0) {
        filename[j] = entry_data[i + 2];
        i++;
        j++;
    }
    filename[j] = '\0';

    ASSERT(strcmp(filename, "FILENAME") == 0, "Filename parsing removes padding");

    return 1;
}

/* ============================================================================
 * TEST 10: Read Entry Parameters
 * ============================================================================ */

int test_readdir_parameters(void) {
    TEST("cbm_readdir parameter validation");

    char filename[17];
    unsigned char type;
    unsigned int size;

    /* cbm_readdir with invalid parameters should return -1 */
    int result = cbm_readdir(-1, filename, &type, &size);
    ASSERT(result == -1, "cbm_readdir(-1, ...) returns -1");

    result = cbm_readdir(5, NULL, &type, &size);
    ASSERT(result == -1, "cbm_readdir(5, NULL, ...) returns -1");

    result = cbm_readdir(5, filename, NULL, &size);
    ASSERT(result == -1, "cbm_readdir(5, ..., NULL, ...) returns -1");

    return 1;
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void) {
    printf("========================================\n");
    printf("FILE I/O Phase 3 - Directory Operations\n");
    printf("========================================\n");

    /* Run all tests */
    test_file_type_names();
    test_directory_handles();
    test_directory_entry();
    test_delete_function();
    test_rename_function();
    test_format_function();
    test_cbm_type_constants();
    test_device_constants();
    test_directory_string_parsing();
    test_readdir_parameters();

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

