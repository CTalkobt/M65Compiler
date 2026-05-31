/**
 * Calling Convention Safety Matrix Test
 *
 * Tests calling convention interactions between stack and ZP conventions.
 * Validates correct parameter passing, return values, and clobber tracking.
 *
 * Usage: cc45 -fzpcall test_cc_safety_matrix.c
 */

#include <stdio.h>
#include <stdlib.h>

/* Stack convention functions (baseline) */
int stack_add(int a, int b) {
    return a + b;
}

long stack_long_add(long a, long b) {
    return a + b;
}

int stack_three_params(int a, int b, int c) {
    return a + b * c;
}

/* ZP convention functions (fastcall) */
__fastcall__ int zp_add(int a, int b) {
    return a + b;
}

__fastcall__ long zp_long_add(long a, long b) {
    return a + b;
}

__fastcall__ int zp_three_params(int a, int b, int c) {
    return a + b * c;
}

/* Mixed convention: ZP caller calls both stack and ZP callees */
__fastcall__ int zp_mixed_caller(int x) {
    int s = stack_add(x, 1);      /* ZP→Stack */
    int z = zp_add(x, 2);         /* ZP→ZP */
    return s + z;
}

/* Test: Stack caller → Stack callee */
void test_stack_to_stack() {
    int r = stack_add(5, 3);
    if (r != 8) {
        printf("FAIL: stack_to_stack\n");
        exit(1);
    }
    printf("PASS: stack_to_stack\n");
}

/* Test: Stack caller → ZP callee */
void test_stack_to_zp() {
    int r = zp_add(7, 2);
    if (r != 9) {
        printf("FAIL: stack_to_zp\n");
        exit(1);
    }
    printf("PASS: stack_to_zp\n");
}

/* Test: ZP caller → ZP callee */
__fastcall__ int zp_test_zp_to_zp(void) {
    int r = zp_add(4, 5);
    return r;
}

void test_zp_to_zp() {
    int r = zp_test_zp_to_zp();
    if (r != 9) {
        printf("FAIL: zp_to_zp\n");
        exit(1);
    }
    printf("PASS: zp_to_zp\n");
}

/* Test: Long (32-bit) return values */
void test_long_returns() {
    long s_r = stack_long_add(0x1000L, 0x2000L);
    long z_r = zp_long_add(0x3000L, 0x4000L);

    if (s_r != 0x3000L || z_r != 0x7000L) {
        printf("FAIL: long_returns\n");
        exit(1);
    }
    printf("PASS: long_returns\n");
}

/* Test: Multiple parameters */
void test_multi_params() {
    int s_r = stack_three_params(2, 3, 4);  /* 2 + 3*4 = 14 */
    int z_r = zp_three_params(2, 3, 4);     /* 2 + 3*4 = 14 */

    if (s_r != 14 || z_r != 14) {
        printf("FAIL: multi_params\n");
        exit(1);
    }
    printf("PASS: multi_params\n");
}

/* Test: Mixed convention calls */
void test_mixed_conventions() {
    int r = zp_mixed_caller(10);
    /* stack_add(10, 1) = 11, zp_add(10, 2) = 12, result = 23 */
    if (r != 23) {
        printf("FAIL: mixed_conventions\n");
        exit(1);
    }
    printf("PASS: mixed_conventions\n");
}

/* Test: Global state access */
static int global_state = 100;

void stack_modify_global(int x) {
    global_state += x;
}

__fastcall__ void zp_modify_global(int x) {
    global_state += x;
}

void test_global_state() {
    global_state = 100;
    stack_modify_global(5);
    if (global_state != 105) {
        printf("FAIL: global_state (stack)\n");
        exit(1);
    }
    zp_modify_global(10);
    if (global_state != 115) {
        printf("FAIL: global_state (zp)\n");
        exit(1);
    }
    printf("PASS: global_state\n");
}

/* Test: Register clobbering */
void test_register_clobbering() {
    int a = 100;
    int b = 200;

    /* Call should not clobber local variables */
    int c = zp_add(a, b);

    if (a != 100 || b != 200 || c != 300) {
        printf("FAIL: register_clobbering\n");
        exit(1);
    }
    printf("PASS: register_clobbering\n");
}

/* Main test driver */
int main(void) {
    printf("=== Calling Convention Safety Matrix ===\n");

    test_stack_to_stack();
    test_stack_to_zp();
    test_zp_to_zp();
    test_long_returns();
    test_multi_params();
    test_mixed_conventions();
    test_global_state();
    test_register_clobbering();

    printf("\n=== All Calling Convention Tests Passed ===\n");
    return 0;
}
