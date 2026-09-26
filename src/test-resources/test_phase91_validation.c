// Phase 91: Cross-Module Optimization Validation Test
// Tests dead code elimination and function inlining
// Expected: 15-20% code size reduction with -O1 vs -O0

#include <stdio.h>

// Leaf function (no calls) - should be inlined
static int leaf_add(int a, int b) {
    return a + b;
}

// Small single-caller function - should be inlined
static int small_helper(int x) {
    return x * 2 + 1;
}

// Unused dead code function - should be eliminated
static int dead_function(int x) {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
    }
    return sum + x;
}

// Function that calls dead_function but is itself unused
static int also_dead(void) {
    return dead_function(42);
}

// Test: Verify inlining and dead code elimination
int main(void) {
    int result = 0;

    // Call leaf function 5 times - should inline all
    for (int i = 0; i < 5; i++) {
        result += leaf_add(i, i + 1);
    }

    // Call small_helper 3 times - should inline all
    result += small_helper(result);
    result += small_helper(result);
    result += small_helper(result);

    // Verify correct computation
    // result = (0+1) + (1+2) + (2+3) + (3+4) + (4+5)
    //        = 1 + 3 + 5 + 7 + 9 = 25
    // Then small_helper(25) = 25*2+1 = 51
    // small_helper(51) = 51*2+1 = 103
    // small_helper(103) = 103*2+1 = 207
    // Final result: 207

    return result;  // Should be 207
}
