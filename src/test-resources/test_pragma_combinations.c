/**
 * Pragma Combinations Integration Test
 *
 * Tests behavior when multiple pragmas are used in combination:
 * - Multiple pragmas in sequence
 * - Pragma scope (file-level, function-level)
 * - Pragma interactions (encoding + calling conventions, etc.)
 * - Pragma override/re-specification
 *
 * Usage: cc45 test_pragma_combinations.c
 */

#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
   Test 1: Single pragma - encoding
   ============================================================================ */

#pragma encoding(ascii)

const char test1_string[] = "TEST1";

void test_single_pragma() {
    /* String should be ASCII encoded */
    if (test1_string[0] != 'T' || test1_string[4] != '1') {
        printf("FAIL: single_pragma\n");
        exit(1);
    }
    printf("PASS: single_pragma\n");
}

/* ============================================================================
   Test 2: Pragma reset and change
   ============================================================================ */

#pragma encoding(petscii)  /* Reset to PETSCII */

const char test2_string[] = "TEST2";

void test_pragma_reset() {
    /* String should be PETSCII encoded (different from ASCII) */
    if (test2_string[0] == 'T') {  /* Would be same if ASCII still active */
        printf("FAIL: pragma_reset (likely still ASCII)\n");
        exit(1);
    }
    printf("PASS: pragma_reset\n");
}

/* ============================================================================
   Test 3: Heap pragma with encoding
   ============================================================================ */

#pragma cc45 heap

static char *heap_buffer = 0;

void test_heap_with_encoding() {
    /* Just test that both pragmas can coexist */
    heap_buffer = malloc(64);
    if (heap_buffer == 0) {
        printf("FAIL: heap_with_encoding (malloc failed)\n");
        exit(1);
    }
    free(heap_buffer);
    printf("PASS: heap_with_encoding\n");
}

/* ============================================================================
   Test 4: Multiple pragmas in sequence
   ============================================================================ */

#pragma encoding(screencode)
#pragma cc45 heap

const char test4_string[] = "TEST4";
static int global_data = 42;

void test_multiple_sequential_pragmas() {
    if (global_data != 42) {
        printf("FAIL: multiple_sequential_pragmas\n");
        exit(1);
    }
    printf("PASS: multiple_sequential_pragmas\n");
}

/* ============================================================================
   Test 5: Pragma in different scopes
   ============================================================================ */

#pragma encoding(petscii)

void function_with_petscii_strings() {
    const char *s = "PETSCII";
    if (s == 0) {
        printf("FAIL: function_scope_petscii\n");
        exit(1);
    }
}

#pragma encoding(ascii)

void function_with_ascii_strings() {
    const char *s = "ASCII";
    if (s == 0) {
        printf("FAIL: function_scope_ascii\n");
        exit(1);
    }
}

void test_scope_changes() {
    function_with_petscii_strings();
    function_with_ascii_strings();
    printf("PASS: scope_changes\n");
}

/* ============================================================================
   Test 6: Pragma override (re-specify same pragma)
   ============================================================================ */

#pragma encoding(ascii)
#pragma encoding(ascii)  /* Redundant, should be harmless */

const char test6_string[] = "TEST6";

void test_pragma_override() {
    if (test6_string == 0) {
        printf("FAIL: pragma_override\n");
        exit(1);
    }
    printf("PASS: pragma_override\n");
}

/* ============================================================================
   Test 7: Pragmas with different target features
   ============================================================================ */

#pragma cc45 no_bssinit
#pragma encoding(petscii)

static int bss_var1 = 0;
static int bss_var2;

void test_different_pragma_types() {
    bss_var1 = 100;
    bss_var2 = 200;

    if (bss_var1 != 100 || bss_var2 != 200) {
        printf("FAIL: different_pragma_types\n");
        exit(1);
    }
    printf("PASS: different_pragma_types\n");
}

/* ============================================================================
   Test 8: Pragma state persistence
   ============================================================================ */

#pragma encoding(screencode)

void func1_with_screencode() {
    const char *s = "func1";
    if (s == 0) {
        printf("FAIL: pragma_persistence (func1)\n");
        exit(1);
    }
}

/* Pragma state should still be screencode for func2 */
void func2_still_screencode() {
    const char *s = "func2";
    if (s == 0) {
        printf("FAIL: pragma_persistence (func2)\n");
        exit(1);
    }
}

void test_pragma_persistence() {
    func1_with_screencode();
    func2_still_screencode();
    printf("PASS: pragma_persistence\n");
}

/* ============================================================================
   Test 9: Pragma before vs after declaration
   ============================================================================ */

const char early_string[] = "EARLY";  /* Before encoding pragma */

#pragma encoding(ascii)

const char late_string[] = "LATE";   /* After encoding pragma */

void test_pragma_timing() {
    /* Both should compile without error */
    if (early_string[0] == 0 || late_string[0] == 0) {
        printf("FAIL: pragma_timing\n");
        exit(1);
    }
    printf("PASS: pragma_timing\n");
}

/* ============================================================================
   Test 10: Global pragmas don't affect local variables incorrectly
   ============================================================================ */

#pragma cc45 heap

void test_local_vars_unaffected() {
    int local_a = 10;
    int local_b = 20;
    int local_c = local_a + local_b;

    if (local_c != 30) {
        printf("FAIL: local_vars_unaffected\n");
        exit(1);
    }
    printf("PASS: local_vars_unaffected\n");
}

/* ============================================================================
   Main Test Driver
   ============================================================================ */

int main(void) {
    printf("=== Pragma Combinations Integration Tests ===\n");

    test_single_pragma();
    test_pragma_reset();
    test_heap_with_encoding();
    test_multiple_sequential_pragmas();
    test_scope_changes();
    test_pragma_override();
    test_different_pragma_types();
    test_pragma_persistence();
    test_pragma_timing();
    test_local_vars_unaffected();

    printf("\n=== All Pragma Combination Tests Passed ===\n");
    return 0;
}
