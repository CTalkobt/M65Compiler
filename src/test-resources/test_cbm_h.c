// Test: cbm.h compile+assemble smoke test (Issue #46)
// Exercises every parameterized KERNAL wrapper to verify inline asm
// parameter references (@_p_xxx) resolve correctly.
// No runtime validation — KERNAL routines not available in mmemu.

#include <cbm.h>

int ch;

// Test 1: global variable, single condition (original working case)
void test_global_single_if(void) {
    ch = cbm_getin();
    if (ch == 0x51) {
        cbm_chrout(ch);
    }
}

// Test 2: local variable, single condition (original failing case)
void test_local_single_if(void) {
    int ch = cbm_getin();
    if (ch == 0x51) {
        cbm_chrout(ch);
    }
}

// Test 3: multiple conditions
void test_multiple_ifs(void) {
    ch = cbm_getin();
    if (ch == 0x51) {
        cbm_chrout(ch);
    }
    if (ch == 0x57) {
        cbm_chrout(ch);
    }
}

// Test 4: exercise every parameterized cbm.h function
void test_all_parameterized(void) {
    cbm_chrout('A');
    cbm_setlfs(1, 8, 15);
    cbm_setnam("TEST", 4);
    cbm_close(1);
    cbm_chkin(1);
    cbm_chkout(1);
    cbm_gotoxy(0, 0);
    cbm_listen(8);
    cbm_talk(8);
    cbm_second(15);
    cbm_tksa(15);
    cbm_ciout(0x41);
}

// Test 5: no-arg functions (should always work, included for completeness)
void test_no_arg(void) {
    cbm_getin();
    cbm_chrin();
    cbm_readst();
    cbm_open();
    cbm_clrchn();
    cbm_clall();
    cbm_stop();
    cbm_wherexy();
    cbm_screen();
    cbm_rdtim();
    cbm_unlisten();
    cbm_untalk();
    cbm_acptr();
}

void main() {
    test_global_single_if();
    test_local_single_if();
    test_multiple_ifs();
    test_all_parameterized();
    test_no_arg();
}
