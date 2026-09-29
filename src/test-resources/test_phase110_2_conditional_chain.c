// Test Phase 110.2: Conditional Jump-Chain Fusion
// Tests fusion of adjacent conditional branches with identical conditions

// =============================================================================
// SIMPLE CONDITIONAL CHAINS
// =============================================================================

// Simple if-else chain: two comparisons of same variable
int test_chain_simple(int x) {
    if (x == 0) {
        return 1;
    }
    if (x == 0) {  // SAME condition as above
        return 2;
    }
    return 3;
}

// Nested conditionals with repeated condition
int test_chain_nested(int a, int b) {
    if (a > 0) {
        if (b > 0) {
            return 1;
        }
    }
    if (a > 0) {  // SAME condition as outer if
        return 2;
    }
    return 3;
}

// Multiple consecutive checks of same variable
int test_chain_multiple(int val) {
    if (val == 5) return 10;
    if (val == 5) return 20;  // Redundant
    if (val == 5) return 30;  // Redundant
    return 0;
}

// =============================================================================
// SWITCH-LIKE PATTERNS (multiple cases of same variable)
// =============================================================================

// Simulated switch with multiple equality checks
int test_switch_pattern(int op) {
    if (op == 1) {
        return 100;
    } else if (op == 2) {
        return 200;
    } else if (op == 3) {
        return 300;
    }
    return 0;
}

// Pattern: check same variable multiple times with different constants
int test_multiple_checks(int code) {
    if (code == 10) return 1;
    if (code == 20) return 2;
    if (code == 30) return 3;
    if (code == 40) return 4;
    return 5;
}

// =============================================================================
// COMPARISON CHAINS
// =============================================================================

// Comparison chain: < operator
int test_comparison_less(int x) {
    if (x < 10) {
        return 1;
    }
    if (x < 10) {  // SAME condition
        return 2;
    }
    return 3;
}

// Comparison chain: > operator
int test_comparison_greater(int x) {
    if (x > 5) {
        return 1;
    }
    if (x > 5) {  // SAME condition
        return 2;
    }
    return 3;
}

// Comparison chain: <= operator
int test_comparison_lte(int x) {
    if (x <= 100) {
        return 1;
    }
    if (x <= 100) {  // SAME condition
        return 2;
    }
    return 3;
}

// Comparison chain: >= operator
int test_comparison_gte(int x) {
    if (x >= 0) {
        return 1;
    }
    if (x >= 0) {  // SAME condition
        return 2;
    }
    return 3;
}

// Comparison chain: != operator
int test_comparison_ne(int x) {
    if (x != 0) {
        return 1;
    }
    if (x != 0) {  // SAME condition
        return 2;
    }
    return 3;
}

// =============================================================================
// LOGICAL OPERATOR CHAINS
// =============================================================================

// AND chain
int test_logical_and(int a, int b) {
    if (a > 0 && b > 0) {
        return 1;
    }
    if (a > 0 && b > 0) {  // SAME condition
        return 2;
    }
    return 3;
}

// OR chain
int test_logical_or(int a, int b) {
    if (a < 0 || b < 0) {
        return 1;
    }
    if (a < 0 || b < 0) {  // SAME condition
        return 2;
    }
    return 3;
}

// =============================================================================
// BITWISE OPERATOR CHAINS
// =============================================================================

// Bitwise AND check (flag testing)
int test_bitwise_and(int flags) {
    if (flags & 0x01) {
        return 1;
    }
    if (flags & 0x01) {  // SAME condition
        return 2;
    }
    return 3;
}

// Bitwise OR check
int test_bitwise_or(int flags) {
    if (flags | 0x02) {
        return 1;
    }
    if (flags | 0x02) {  // SAME condition
        return 2;
    }
    return 3;
}

// =============================================================================
// NON-CHAINABLE PATTERNS (should NOT fuse)
// =============================================================================

// Different conditions: should NOT fuse
int test_different_conditions(int x) {
    if (x == 0) {
        return 1;
    }
    if (x == 5) {  // DIFFERENT condition
        return 2;
    }
    return 3;
}

// Same variable but different operators: should NOT fuse
int test_different_operators(int x) {
    if (x > 0) {
        return 1;
    }
    if (x >= 0) {  // Different operator (>= vs >)
        return 2;
    }
    return 3;
}

// Variable modified between checks: should NOT fuse
int test_modified_between(int x) {
    if (x == 0) {
        return 1;
    }
    x = x + 1;  // Variable modified!
    if (x == 0) {  // Condition changes meaning
        return 2;
    }
    return 3;
}

// Side effects between: should NOT fuse
int global_var = 0;

int test_side_effects(int x) {
    if (x == 0) {
        return 1;
    }
    global_var++;  // Side effect!
    if (x == 0) {  // Could be affected
        return 2;
    }
    return 3;
}

// =============================================================================
// REAL-WORLD PATTERNS
// =============================================================================

// Error code checking
int handle_error(int error_code) {
    if (error_code == -1) {
        return 100;  // File not found
    }
    if (error_code == -2) {
        return 200;  // Permission denied
    }
    if (error_code == -3) {
        return 300;  // Invalid argument
    }
    return 0;  // Success
}

// State machine pattern
int process_state(int state) {
    if (state == 0) {
        return 10;  // IDLE
    }
    if (state == 1) {
        return 20;  // RUNNING
    }
    if (state == 2) {
        return 30;  // PAUSED
    }
    if (state == 3) {
        return 40;  // STOPPED
    }
    return -1;  // UNKNOWN
}

// Flag decoder
int decode_flags(int flags) {
    int result = 0;
    if (flags & 0x01) result += 1;  // Check flag 0
    if (flags & 0x02) result += 2;  // Check flag 1
    if (flags & 0x04) result += 4;  // Check flag 2
    if (flags & 0x08) result += 8;  // Check flag 3
    return result;
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main() {
    int total = 0;

    // Simple chains
    total += test_chain_simple(0);      // 1
    total += test_chain_nested(1, 1);   // 1
    total += test_chain_multiple(5);    // 10
    total += test_switch_pattern(2);    // 200

    // Comparison chains
    total += test_comparison_less(5);   // 1
    total += test_comparison_greater(10); // 1
    total += test_comparison_lte(50);   // 1
    total += test_comparison_gte(5);    // 1
    total += test_comparison_ne(5);     // 1

    // Logical operators
    total += test_logical_and(1, 1);    // 1
    total += test_logical_or(-1, 5);    // 1

    // Bitwise operators
    total += test_bitwise_and(0x03);    // 1
    total += test_bitwise_or(0x02);     // 1

    // Non-chainable (should work but not optimize)
    total += test_different_conditions(0);  // 1
    total += test_different_operators(1);   // 1
    total += test_modified_between(0);      // 1

    // Real-world patterns
    total += handle_error(-1);              // 100
    total += process_state(2);              // 30
    total += decode_flags(0x0F);            // 15

    return total;  // Should be ~363 if all tests pass
}
