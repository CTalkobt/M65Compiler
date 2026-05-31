/**
 * Inline Assembly Clobber Tracking Integration Test
 *
 * Validates that inline assembly clobber directives are correctly tracked
 * and that the optimizer respects clobber information. If clobber tracking
 * is wrong, the optimizer might incorrectly reuse registers that were
 * actually modified by inline asm, causing memory corruption.
 *
 * Usage: cc45 test_asm_clobber_integration.c
 */

#include <stdio.h>
#include <stdlib.h>

/* Test 1: X register clobbering */
void test_x_clobber() {
    int x = 100;
    int y = 200;

    asm("ldx #$42");  /* Clobber X */

    /* x and y must be unaffected */
    if (x != 100 || y != 200) {
        printf("FAIL: x_clobber\n");
        exit(1);
    }
    printf("PASS: x_clobber\n");
}

/* Test 2: A register clobbering */
void test_a_clobber() {
    int a = 5;
    int b = 10;
    int c = 15;

    asm("lda #$FF");  /* Clobber A */

    if (a != 5 || b != 10 || c != 15) {
        printf("FAIL: a_clobber\n");
        exit(1);
    }
    printf("PASS: a_clobber\n");
}

/* Test 3: Y register clobbering */
void test_y_clobber() {
    int y_var = 42;
    int z_var = 84;

    asm("ldy #$30");  /* Clobber Y */

    if (y_var != 42 || z_var != 84) {
        printf("FAIL: y_clobber\n");
        exit(1);
    }
    printf("PASS: y_clobber\n");
}

/* Test 4: Multiple register clobbering */
void test_multi_register_clobber() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;

    asm("lda #$11; ldx #$22; ldy #$33");  /* Clobber A, X, Y */

    if (a != 1 || b != 2 || c != 3 || d != 4) {
        printf("FAIL: multi_register_clobber\n");
        exit(1);
    }
    printf("PASS: multi_register_clobber\n");
}

/* Test 5: Clobber with variable access in asm */
void test_asm_variable_access() {
    int counter = 0;
    int limit = 5;

    /* Access variable @_l_counter in asm (if supported) */
    asm("ldx #$00");  /* Clobber X, but counter should be preserved */

    counter++;

    if (counter != 1 || limit != 5) {
        printf("FAIL: asm_variable_access\n");
        exit(1);
    }
    printf("PASS: asm_variable_access\n");
}

/* Test 6: Nested asm blocks */
void test_nested_asm() {
    int value = 123;

    asm("lda #$01");  /* Clobber A */
    value = value + 1;
    asm("ldx #$02");  /* Clobber X */
    value = value * 2;

    if (value != 248) {  /* (123 + 1) * 2 = 248 */
        printf("FAIL: nested_asm\n");
        exit(1);
    }
    printf("PASS: nested_asm\n");
}

/* Test 7: Asm in loop */
void test_asm_in_loop() {
    int result = 0;

    for (int i = 0; i < 5; i++) {
        asm("lda #$00");  /* Clobber A each iteration */
        result += i;
    }

    if (result != 10) {  /* 0+1+2+3+4 = 10 */
        printf("FAIL: asm_in_loop\n");
        exit(1);
    }
    printf("PASS: asm_in_loop\n");
}

/* Test 8: Asm with function calls */
void helper_function() {
    /* Empty helper */
}

void test_asm_with_call() {
    int x = 50;

    asm("lda #$FF");  /* Clobber A */
    helper_function();
    int y = x + 1;

    if (x != 50 || y != 51) {
        printf("FAIL: asm_with_call\n");
        exit(1);
    }
    printf("PASS: asm_with_call\n");
}

/* Test 9: Empty asm statement */
void test_empty_asm() {
    int x = 99;
    asm("");  /* Empty asm should not affect anything */
    if (x != 99) {
        printf("FAIL: empty_asm\n");
        exit(1);
    }
    printf("PASS: empty_asm\n");
}

/* Test 10: Asm with whitespace variations */
void test_asm_whitespace() {
    int value = 77;

    asm("   lda #$55   ");  /* Asm with extra whitespace */
    asm("\tldy #$66\t");    /* Asm with tabs */

    if (value != 77) {
        printf("FAIL: asm_whitespace\n");
        exit(1);
    }
    printf("PASS: asm_whitespace\n");
}

int main(void) {
    printf("=== Inline Assembly Clobber Integration Tests ===\n");

    test_x_clobber();
    test_a_clobber();
    test_y_clobber();
    test_multi_register_clobber();
    test_asm_variable_access();
    test_nested_asm();
    test_asm_in_loop();
    test_asm_with_call();
    test_empty_asm();
    test_asm_whitespace();

    printf("\n=== All Clobber Tracking Tests Passed ===\n");
    return 0;
}
