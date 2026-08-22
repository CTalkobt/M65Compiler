// Test case for TableDrivenDispatch optimization
// Phase C4.3: Jump table generation for dense switch statements
//
// Expected behavior:
// - Dense switch statements (10+ cases, 60%+ fill) should use jump tables
// - Reduces code size by eliminating cascading comparisons
// - Improves performance via direct dispatch
// - No behavioral change (results must be identical)

// Test 1: Dense switch with 10 consecutive cases
int test_dense_consecutive(int selector) {
    switch (selector) {
        case 0: return 100;
        case 1: return 101;
        case 2: return 102;
        case 3: return 103;
        case 4: return 104;
        case 5: return 105;
        case 6: return 106;
        case 7: return 107;
        case 8: return 108;
        case 9: return 109;
        default: return 0;
    }
}

// Test 2: Dense switch with gaps (but 60%+ fill)
int test_dense_with_gaps(int selector) {
    switch (selector) {
        case 0: return 0;
        case 1: return 10;
        case 2: return 20;
        case 3: return 30;
        case 5: return 50;      // Gap at 4
        case 6: return 60;
        case 7: return 70;
        case 8: return 80;
        case 9: return 90;
        case 10: return 100;
        default: return -1;
    }
}

// Test 3: Sparse switch (should NOT use table)
int test_sparse_switch(int selector) {
    switch (selector) {
        case 0: return 0;
        case 100: return 100;
        case 200: return 200;
        case 300: return 300;
        default: return -1;
    }
}

// Test 4: Small switch (should NOT use table - fewer than 5 cases)
int test_small_switch(int selector) {
    switch (selector) {
        case 1: return 10;
        case 2: return 20;
        case 3: return 30;
        default: return 0;
    }
}

// Test 5: Switch with character ranges (0-255 range, dense)
int test_char_switch(unsigned char ch) {
    switch (ch) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default: return -1;
    }
}

// Test 6: Switch with default case
int test_switch_with_default(int x) {
    switch (x) {
        case 10: return 100;
        case 11: return 110;
        case 12: return 120;
        case 13: return 130;
        case 14: return 140;
        case 15: return 150;
        case 16: return 160;
        case 17: return 170;
        case 18: return 180;
        case 19: return 190;
        default: return 999;
    }
}

// Test 7: Switch returning computed values
int test_computed_switch(int case_num) {
    switch (case_num) {
        case 0: return 0 * 10;
        case 1: return 1 * 10;
        case 2: return 2 * 10;
        case 3: return 3 * 10;
        case 4: return 4 * 10;
        case 5: return 5 * 10;
        case 6: return 6 * 10;
        case 7: return 7 * 10;
        case 8: return 8 * 10;
        case 9: return 9 * 10;
        default: return -1;
    }
}

// Test 8: Nested switches
int test_nested_switches(int outer, int inner) {
    switch (outer) {
        case 0:
            switch (inner) {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                default: return -1;
            }
        case 1:
            switch (inner) {
                case 0: return 10;
                case 1: return 11;
                case 2: return 12;
                default: return -1;
            }
        default: return -1;
    }
    return -1;
}

// Test 9: Very dense switch (high fill ratio)
int test_very_dense(int x) {
    switch (x) {
        case 0: return 0;
        case 1: return 1;
        case 2: return 2;
        case 3: return 3;
        case 4: return 4;
        case 5: return 5;
        case 6: return 6;
        case 7: return 7;
        case 8: return 8;
        case 9: return 9;
        case 10: return 10;
        case 11: return 11;
        case 12: return 12;
        case 13: return 13;
        case 14: return 14;
        default: return -1;
    }
}

int main() {
    // Test 1: Dense consecutive
    if (test_dense_consecutive(0) != 100) return -1;
    if (test_dense_consecutive(5) != 105) return -1;
    if (test_dense_consecutive(9) != 109) return -1;
    if (test_dense_consecutive(10) != 0) return -1;

    // Test 2: Dense with gaps
    if (test_dense_with_gaps(0) != 0) return -2;
    if (test_dense_with_gaps(5) != 50) return -2;
    if (test_dense_with_gaps(10) != 100) return -2;
    if (test_dense_with_gaps(4) != -1) return -2;  // Gap case

    // Test 3: Sparse (should work but not optimized)
    if (test_sparse_switch(0) != 0) return -3;
    if (test_sparse_switch(100) != 100) return -3;
    if (test_sparse_switch(50) != -1) return -3;

    // Test 4: Small switch
    if (test_small_switch(1) != 10) return -4;
    if (test_small_switch(3) != 30) return -4;
    if (test_small_switch(0) != 0) return -4;

    // Test 5: Char switch
    if (test_char_switch('0') != 0) return -5;
    if (test_char_switch('5') != 5) return -5;
    if (test_char_switch('A') != -1) return -5;

    // Test 6: Switch with default
    if (test_switch_with_default(10) != 100) return -6;
    if (test_switch_with_default(19) != 190) return -6;
    if (test_switch_with_default(20) != 999) return -6;

    // Test 7: Computed switch
    if (test_computed_switch(0) != 0) return -7;
    if (test_computed_switch(5) != 50) return -7;
    if (test_computed_switch(10) != -1) return -7;

    // Test 8: Nested switches
    if (test_nested_switches(0, 0) != 0) return -8;
    if (test_nested_switches(0, 2) != 2) return -8;
    if (test_nested_switches(1, 1) != 11) return -8;

    // Test 9: Very dense
    if (test_very_dense(0) != 0) return -9;
    if (test_very_dense(7) != 7) return -9;
    if (test_very_dense(14) != 14) return -9;
    if (test_very_dense(15) != -1) return -9;

    return 0;  // All tests passed
}
