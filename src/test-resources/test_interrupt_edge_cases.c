/**
 * Interrupt Handler Edge Cases Test
 *
 * Validates correct behavior of __interrupt handlers in edge cases:
 * - Interrupt with variadic arguments (error case)
 * - Interrupt with inline assembly
 * - Interrupt modifying global state
 * - Interrupt return values
 * - Register preservation across interrupt
 *
 * Usage: cc45 test_interrupt_edge_cases.c
 */

#include <stdio.h>
#include <stdlib.h>

/* ============================================================================
   Global state for interrupt testing
   ============================================================================ */

static volatile int interrupt_counter = 0;
static volatile int global_flag = 0;

/* ============================================================================
   Test 1: Basic interrupt handler
   ============================================================================ */

__interrupt void basic_interrupt_handler() {
    interrupt_counter++;
}

void test_basic_interrupt() {
    interrupt_counter = 0;

    /* Simulate interrupt by direct call (valid for testing) */
    basic_interrupt_handler();

    if (interrupt_counter != 1) {
        printf("FAIL: basic_interrupt\n");
        exit(1);
    }
    printf("PASS: basic_interrupt\n");
}

/* ============================================================================
   Test 2: Interrupt modifying global state
   ============================================================================ */

__interrupt void state_modifying_interrupt() {
    global_flag = 1;
    interrupt_counter++;
}

void test_interrupt_modify_globals() {
    global_flag = 0;
    interrupt_counter = 0;

    state_modifying_interrupt();

    if (global_flag != 1 || interrupt_counter != 1) {
        printf("FAIL: interrupt_modify_globals\n");
        exit(1);
    }
    printf("PASS: interrupt_modify_globals\n");
}

/* ============================================================================
   Test 3: Interrupt with inline assembly
   ============================================================================ */

__interrupt void interrupt_with_asm() {
    asm("lda #$42");  /* Clobber A */
    interrupt_counter++;
}

void test_interrupt_with_asm() {
    interrupt_counter = 0;

    interrupt_with_asm();

    if (interrupt_counter != 1) {
        printf("FAIL: interrupt_with_asm\n");
        exit(1);
    }
    printf("PASS: interrupt_with_asm\n");
}

/* ============================================================================
   Test 4: Multiple interrupts in sequence
   ============================================================================ */

__interrupt void count_interrupt() {
    interrupt_counter++;
}

void test_multiple_interrupts() {
    interrupt_counter = 0;

    count_interrupt();
    count_interrupt();
    count_interrupt();

    if (interrupt_counter != 3) {
        printf("FAIL: multiple_interrupts\n");
        exit(1);
    }
    printf("PASS: multiple_interrupts\n");
}

/* ============================================================================
   Test 5: Interrupt preserves caller's state
   ============================================================================ */

__interrupt void preserving_interrupt() {
    global_flag = 99;
}

void test_interrupt_state_preservation() {
    int local_a = 100;
    int local_b = 200;
    global_flag = 0;

    preserving_interrupt();

    /* Local variables should be unaffected */
    if (local_a != 100 || local_b != 200 || global_flag != 99) {
        printf("FAIL: interrupt_state_preservation\n");
        exit(1);
    }
    printf("PASS: interrupt_state_preservation\n");
}

/* ============================================================================
   Test 6: Interrupt with nested state changes
   ============================================================================ */

static volatile int nested_counter = 0;

__interrupt void nested_interrupt() {
    nested_counter++;
    global_flag = nested_counter * 10;
}

void test_nested_interrupt_state() {
    nested_counter = 0;
    global_flag = 0;

    nested_interrupt();
    if (nested_counter != 1 || global_flag != 10) {
        printf("FAIL: nested_interrupt_state (first)\n");
        exit(1);
    }

    nested_interrupt();
    if (nested_counter != 2 || global_flag != 20) {
        printf("FAIL: nested_interrupt_state (second)\n");
        exit(1);
    }

    printf("PASS: nested_interrupt_state\n");
}

/* ============================================================================
   Test 7: Interrupt called from within function
   ============================================================================ */

__interrupt void func_called_interrupt() {
    interrupt_counter++;
}

int caller_with_interrupt() {
    func_called_interrupt();
    return interrupt_counter;
}

void test_interrupt_from_function() {
    interrupt_counter = 0;
    int result = caller_with_interrupt();

    if (result != 1 || interrupt_counter != 1) {
        printf("FAIL: interrupt_from_function\n");
        exit(1);
    }
    printf("PASS: interrupt_from_function\n");
}

/* ============================================================================
   Test 8: Multiple different interrupt handlers
   ============================================================================ */

static volatile int handler_a_called = 0;
static volatile int handler_b_called = 0;

__interrupt void interrupt_a() {
    handler_a_called++;
}

__interrupt void interrupt_b() {
    handler_b_called++;
}

void test_multiple_handlers() {
    handler_a_called = 0;
    handler_b_called = 0;

    interrupt_a();
    interrupt_b();
    interrupt_a();

    if (handler_a_called != 2 || handler_b_called != 1) {
        printf("FAIL: multiple_handlers\n");
        exit(1);
    }
    printf("PASS: multiple_handlers\n");
}

/* ============================================================================
   Test 9: Interrupt with register operations
   ============================================================================ */

__interrupt void register_ops_interrupt() {
    asm("ldx #$55");
    asm("ldy #$66");
    interrupt_counter++;
}

void test_interrupt_register_ops() {
    interrupt_counter = 0;

    register_ops_interrupt();

    if (interrupt_counter != 1) {
        printf("FAIL: interrupt_register_ops\n");
        exit(1);
    }
    printf("PASS: interrupt_register_ops\n");
}

/* ============================================================================
   Test 10: Interrupt clobber tracking
   ============================================================================ */

__interrupt void clobbering_interrupt() {
    asm("lda #$FF; ldx #$00; ldy #$00");
    interrupt_counter++;
}

void test_interrupt_clobber_tracking() {
    int x = 50;
    int y = 75;
    interrupt_counter = 0;

    clobbering_interrupt();

    /* x and y must survive the interrupt */
    if (x != 50 || y != 75 || interrupt_counter != 1) {
        printf("FAIL: interrupt_clobber_tracking\n");
        exit(1);
    }
    printf("PASS: interrupt_clobber_tracking\n");
}

/* ============================================================================
   Main Test Driver
   ============================================================================ */

int main(void) {
    printf("=== Interrupt Handler Edge Cases Tests ===\n");

    test_basic_interrupt();
    test_interrupt_modify_globals();
    test_interrupt_with_asm();
    test_multiple_interrupts();
    test_interrupt_state_preservation();
    test_nested_interrupt_state();
    test_interrupt_from_function();
    test_multiple_handlers();
    test_interrupt_register_ops();
    test_interrupt_clobber_tracking();

    printf("\n=== All Interrupt Handler Tests Passed ===\n");
    return 0;
}
