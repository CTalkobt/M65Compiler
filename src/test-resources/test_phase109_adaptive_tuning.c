// Phase 109.7: Adaptive Optimization Tuning Validation
// Comprehensive test suite for all Phase 109 components

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test 1: Tiny file (should use -O1)
int tiny_function(void) {
    int x = 5;
    int y = 10;
    return x + y;
}

// Test 2: Small file with loops (should detect loop patterns)
int small_loop_sum(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += i;
    }
    return sum;
}

// Test 3: Medium file with complex control flow
int medium_fibonacci(int n) {
    if (n <= 1) return n;

    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// Test 4: Medium file with multiple functions
int helper_func1(int x) {
    return x * 2;
}

int helper_func2(int x) {
    return x + 1;
}

int medium_multi_func(int x) {
    x = helper_func1(x);
    x = helper_func2(x);
    return x;
}

// Test 5: Large file with heavy computation
int large_nested_loops(int m, int n) {
    int result = 0;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
        }
    }
    return result;
}

// Test 6: Large file with bitwise operations (good for strength reduction)
int large_bitwise(int x) {
    int result = 0;

    // Multiplications by powers of 2
    result += x * 2;    // Should become x << 1
    result += x * 4;    // Should become x << 2
    result += x * 8;    // Should become x << 3
    result += x * 16;   // Should become x << 4

    // Divisions by powers of 2
    result += x / 2;    // Should become x >> 1
    result += x / 4;    // Should become x >> 2
    result += x / 8;    // Should become x >> 3

    return result;
}

// Test 7: Large file with dead code (should be eliminated)
int large_dead_code(int x) {
    int unused1 = 100;  // Dead
    int unused2 = 200;  // Dead

    if (x < 0) {
        return -1;
    }

    int unreachable = 300;  // Potentially unreachable

    return x * 2;
}

// Test 8: High complexity - many branches
int high_complexity_branches(int a, int b, int c) {
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                return a + b + c;
            } else {
                return a + b;
            }
        } else {
            if (c > 0) {
                return a + c;
            } else {
                return a;
            }
        }
    } else {
        if (b > 0) {
            if (c > 0) {
                return b + c;
            } else {
                return b;
            }
        } else {
            return c;
        }
    }
}

// Test 9: High complexity - many loops
int high_complexity_loops(int n) {
    int result = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            for (int k = 0; k < j; k++) {
                result++;
            }
        }
    }

    return result;
}

// Test 10: Array indexing (good for striped array optimization)
int array_indexing(void) {
    int arr[10][10];

    // Initialize array
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            arr[i][j] = i * j;
        }
    }

    // Sum diagonal
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += arr[i][i];
    }

    return sum;
}

// Test 11: Constant folding candidate
int constant_folding(void) {
    // All constants should be folded at compile time
    int x = 5 + 3;           // Should become 8
    int y = x * 2;           // Should become 16
    int z = y / 2;           // Should become 8
    int w = z + x + y;       // Should become 32

    return w;
}

// Test 12: Inlining candidate (small function called multiple times)
__inline int inline_candidate(int x) {
    return x + 1;
}

int inline_test(int x) {
    int a = inline_candidate(x);
    int b = inline_candidate(a);
    int c = inline_candidate(b);
    return c;
}

// Test 13: CSE (Common Subexpression Elimination) candidate
int cse_candidate(int x, int y) {
    int a = (x + y) * 2;
    int b = (x + y) * 3;  // (x+y) computed again, could be CSE
    int c = (x + y) + 1;  // (x+y) computed again, could be CSE

    return a + b + c;
}

// Test 14: Loop invariant code motion candidate
int licm_candidate(int n, int c) {
    int sum = 0;
    int invariant = c * 10;  // Computed in loop, but loop-invariant

    for (int i = 0; i < n; i++) {
        sum += i + invariant;
    }

    return sum;
}

// Test 15: Branch folding candidate
int branch_folding(int x) {
    if (1) {  // Always true
        return x + 1;
    } else {
        return x - 1;  // Dead code
    }
}

int main(void) {
    // Test outputs
    printf("Tiny function: %d\n", tiny_function());
    printf("Small loop sum(5): %d\n", small_loop_sum(5));
    printf("Medium fibonacci(10): %d\n", medium_fibonacci(10));
    printf("Medium multi func(5): %d\n", medium_multi_func(5));
    printf("Large nested loops(10,10): %d\n", large_nested_loops(10, 10));
    printf("Large bitwise(5): %d\n", large_bitwise(5));
    printf("Large dead code(5): %d\n", large_dead_code(5));
    printf("High complexity branches(1,2,3): %d\n",
           high_complexity_branches(1, 2, 3));
    printf("High complexity loops(5): %d\n", high_complexity_loops(5));
    printf("Array indexing: %d\n", array_indexing());
    printf("Constant folding: %d\n", constant_folding());
    printf("Inline test(1): %d\n", inline_test(1));
    printf("CSE candidate(2,3): %d\n", cse_candidate(2, 3));
    printf("LICM candidate(10,5): %d\n", licm_candidate(10, 5));
    printf("Branch folding(5): %d\n", branch_folding(5));

    return 0;
}
