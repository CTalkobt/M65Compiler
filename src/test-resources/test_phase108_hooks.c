// Phase 108.4: Compiler Tuning Hooks - Comprehensive Validation
// Tests all 12 hook points, signal collection, and decision logic

#include <stdio.h>
#include <stdlib.h>

// Test 1: Simple function (baseline signal collection)
int simple_add(int a, int b) {
    return a + b;
}

// Test 2: Function with branches (function analysis signal)
int simple_max(int x, int y) {
    if (x > y) {
        return x;
    } else {
        return y;
    }
}

// Test 3: Small loop (loop analysis signal)
int loop_sum(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += i;
    }
    return sum;
}

// Test 4: Constant folding candidate
int const_fold(void) {
    int x = 5 + 3;
    int y = x * 2;
    int z = 10 / 2;
    return x + y + z;
}

// Test 5: Function call analysis
int helper_a(int x) {
    return x + 1;
}

int helper_b(int x) {
    return x * 2;
}

int complex_call(int x) {
    int a = helper_a(x);
    int b = helper_b(a);
    return a + b;
}

// Test 6: Nested loops
int nested_loops(int m, int n) {
    int result = 0;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            result += i * j;
        }
    }
    return result;
}

// Test 7: Many branches
int branch_heavy(int a, int b, int c, int d) {
    if (a > 0) {
        if (b > 0) {
            if (c > 0) {
                if (d > 0) {
                    return a + b + c + d;
                }
            }
        }
    }
    return 0;
}

// Test 8: Inline candidate
__inline int small_mult(int x) {
    return x * 2;
}

int inline_test(int x) {
    int a = small_mult(x);
    int b = small_mult(a);
    int c = small_mult(b);
    return c;
}

// Test 9: Dead code
int dead_code(int x) {
    if (x < 0) {
        return -1;
    }
    int unused = 100;
    return x * 2;
}

// Test 10: Array access
int array_sum(void) {
    int arr[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 2;
    }
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += arr[i];
    }
    return sum;
}

// Test 11: Recursive function
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Test 12: Mixed complexity
int mixed_complex(int n, int m) {
    int result = 0;
    for (int i = 0; i < n; i++) {
        if (i % 2 == 0) {
            result += i;
        } else {
            result += i * 2;
        }
    }
    result += helper_a(result);
    result += helper_b(result);
    for (int j = 0; j < m; j++) {
        result += j;
    }
    return result;
}

// Test 13: Volatile access
volatile int global_counter = 0;

int volatile_access(void) {
    global_counter++;
    global_counter++;
    int val = global_counter;
    global_counter--;
    return val;
}

// Test 14-18: Multiple functions
int func_14(int x) { return x + 1; }
int func_15(int x) { return x + 2; }
int func_16(int x) { return x + 3; }
int func_17(int x) { return x + 4; }
int func_18(int x) { return x + 5; }

// Test 19: Struct with member access
struct Point {
    int x;
    int y;
};

int point_sum(struct Point p) {
    return p.x + p.y;
}

// Main test
int main(void) {
    printf("=== Phase 108 Hook Test Suite ===\n\n");
    printf("Test 1 - simple_add(3, 4) = %d\n", simple_add(3, 4));
    printf("Test 2 - simple_max(5, 3) = %d\n", simple_max(5, 3));
    printf("Test 3 - loop_sum(5) = %d\n", loop_sum(5));
    printf("Test 4 - const_fold() = %d\n", const_fold());
    printf("Test 5 - complex_call(2) = %d\n", complex_call(2));
    printf("Test 6 - nested_loops(3, 4) = %d\n", nested_loops(3, 4));
    printf("Test 7 - branch_heavy(1, 2, 3, 4) = %d\n", branch_heavy(1, 2, 3, 4));
    printf("Test 8 - inline_test(2) = %d\n", inline_test(2));
    printf("Test 9 - dead_code(5) = %d\n", dead_code(5));
    printf("Test 10 - array_sum() = %d\n", array_sum());
    printf("Test 11 - fibonacci(6) = %d\n", fibonacci(6));
    printf("Test 12 - mixed_complex(5, 3) = %d\n", mixed_complex(5, 3));
    printf("Test 13 - volatile_access() = %d\n", volatile_access());
    printf("Test 14-18 sum = %d\n",
           func_14(1) + func_15(1) + func_16(1) + func_17(1) + func_18(1));
    struct Point p = {3, 4};
    printf("Test 19 - point_sum({3, 4}) = %d\n", point_sum(p));
    printf("\n=== All Tests Complete ===\n");
    return 0;
}
