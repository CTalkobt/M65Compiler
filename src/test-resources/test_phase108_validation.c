// Phase 108.4: Validation & Measurement Test Suite
// Tests hook effectiveness, decision quality, and performance impact

#include <stdio.h>
#include <string.h>

// ============================================================================
// Test 1: Small File (Tight Budget Scenario)
// Expected: Phase 108 should disable expensive optimizations
// ============================================================================

int small_file_test_1() {
    return 42;
}

int small_file_test_2() {
    int x = 10;
    return x + 5;
}

// ============================================================================
// Test 2: Medium File (Budget Adequate)
// Expected: Phase 108 should enable standard optimizations
// ============================================================================

int medium_complexity(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum = sum + i * 2;
    }
    return sum;
}

int medium_optimization_candidate(int x, int y) {
    int a = x + 1;
    int b = x + 1;
    int c = x + 1;
    return a + b + c;
}

// ============================================================================
// Test 3: Large File (Budget Generous)
// Expected: Phase 108 should enable all optimizations including inlining
// ============================================================================

int large_inline_candidate(int x) {
    return x * 2 + 1;
}

int large_optimizable(int a, int b, int c) {
    int t1 = a + b;
    int t2 = b + c;
    int t3 = c + a;
    int r1 = t1 + t2;
    int r2 = t2 + t3;
    return r1 + r2;
}

int large_loop_candidate(int n) {
    int result = 0;
    for (int i = 0; i < n; i++) {
        result += i;
        result *= 2;
        result -= 1;
    }
    return result;
}

// ============================================================================
// Test 4: Decision Effectiveness
// Tests whether hooks correctly identify optimization opportunities
// ============================================================================

int decision_test_cse() {
    // Common subexpression elimination candidate
    int x = 100;
    int y = x + 50;
    int z = x + 50;  // Same expression as y
    return y + z;
}

int decision_test_constant_propagation() {
    // Constant propagation candidate
    int a = 42;
    int b = a;
    int c = b;
    int d = c;
    return d;
}

int decision_test_dead_code() {
    // Dead code elimination candidate
    int unused = 999;
    int used = 123;
    return used;
}

// ============================================================================
// Test 5: Function Inlining Effectiveness
// Expected: Phase 108 should enable inlining for small functions
// ============================================================================

static inline int inline_tiny(int x) {
    return x + 1;
}

static inline int inline_small(int a, int b) {
    return a + b + 1;
}

int test_inlining_calls() {
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total += inline_tiny(i);
        total += inline_small(i, i+1);
    }
    return total;
}

// ============================================================================
// Test 6: Register Pressure Estimation
// Expected: Phase 108 should adjust strategy based on register usage
// ============================================================================

int high_register_pressure(int a, int b, int c, int d, int e, int f) {
    int r1 = a + b;
    int r2 = c + d;
    int r3 = e + f;
    int r4 = r1 + r2;
    int r5 = r2 + r3;
    int r6 = r3 + r1;
    return r4 + r5 + r6;
}

// ============================================================================
// Test 7: Branch Density Analysis
// Expected: Phase 108 should consider branch complexity in decisions
// ============================================================================

int high_branch_density(int x, int y, int z) {
    if (x > 0) {
        if (y > 0) {
            if (z > 0) {
                return x + y + z;
            } else {
                return x + y;
            }
        } else {
            return x;
        }
    } else {
        return 0;
    }
}

// ============================================================================
// Test 8: Array Indexing (Performance Critical)
// Expected: Phase 108 should prioritize optimization for array-heavy code
// ============================================================================

int array_access_test(int arr[100], int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

// ============================================================================
// Test 9: Pointer Arithmetic
// Expected: Phase 108 should handle pointer optimization decisions
// ============================================================================

int pointer_arithmetic_test(int* ptr, int len) {
    int result = 0;
    for (int i = 0; i < len; i++) {
        result += *ptr;
        ptr++;
    }
    return result;
}

// ============================================================================
// Test 10: Mixed Optimization Scenarios
// Combines multiple optimization opportunities
// ============================================================================

struct Point {
    int x;
    int y;
};

int mixed_optimization(struct Point* points, int count) {
    int total = 0;
    for (int i = 0; i < count; i++) {
        int px = points[i].x;
        int py = points[i].y;
        int dist_sq = px * px + py * py;
        total += dist_sq;
    }
    return total;
}

// ============================================================================
// Main entry point - calls all tests
// ============================================================================

int main() {
    int result = 0;

    // Test 1: Small file
    result += small_file_test_1();
    result += small_file_test_2();

    // Test 2: Medium file
    result += medium_complexity(10);
    result += medium_optimization_candidate(5);

    // Test 3: Large file
    result += large_inline_candidate(10);
    result += large_optimizable(1, 2, 3);
    result += large_loop_candidate(5);

    // Test 4: Decision effectiveness
    result += decision_test_cse();
    result += decision_test_constant_propagation();
    result += decision_test_dead_code();

    // Test 5: Inlining effectiveness
    result += test_inlining_calls();

    // Test 6: Register pressure
    result += high_register_pressure(1, 2, 3, 4, 5, 6);

    // Test 7: Branch density
    result += high_branch_density(1, 2, 3);

    // Test 8: Array access
    int test_arr[5] = {1, 2, 3, 4, 5};
    result += array_access_test(test_arr, 5);

    // Test 9: Pointer arithmetic
    result += pointer_arithmetic_test(test_arr, 5);

    // Test 10: Mixed optimization
    struct Point pts[2] = {{3, 4}, {5, 12}};
    result += mixed_optimization(pts, 2);

    return result % 256;
}
