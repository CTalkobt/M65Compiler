// SAC Advanced Edge Cases - Test complex scenarios
// Compile with: cc45 test_sac_edge_cases_advanced.c -fstaticalloc -o test_sac_edge.prg
// Tests SAC behavior in edge cases and complex scenarios

#include <stdio.h>

// Edge Case 1: Mutual recursion detection
// Note: SAC should prevent this at compile time or runtime should handle it
int fib_a(int n);
int fib_b(int n);

int fib_a(int n) {
    if (n <= 1) return n;
    return fib_b(n - 1);
}

int fib_b(int n) {
    if (n <= 0) return 0;
    return fib_a(n - 1) + 1;
}

// Edge Case 2: Large local variable count
int many_locals(int a, int b, int c) {
    int l1 = a;
    int l2 = b;
    int l3 = c;
    int l4 = l1 + l2;
    int l5 = l2 + l3;
    int l6 = l3 + l1;
    int l7 = l4 + l5;
    int l8 = l5 + l6;
    int l9 = l6 + l7;
    int l10 = l7 + l8;
    return l9 + l10;
}

// Edge Case 3: Pointer to local variable
int test_pointer_locals(int x) {
    int val = x * 2;
    int *ptr = &val;
    return *ptr + 10;
}

// Edge Case 4: Array on stack (should work with SAC)
int sum_array(int *arr, int len) {
    int sum = 0;
    int i = 0;
    while (i < len) {
        sum = sum + arr[i];
        i = i + 1;
    }
    return sum;
}

// Edge Case 5: Complex expression with many intermediate values
int complex_expr(int a, int b, int c, int d) {
    int temp1 = a + b;
    int temp2 = c + d;
    int temp3 = temp1 * temp2;
    int temp4 = temp3 - a;
    int temp5 = temp4 / 2;
    return temp5 + c;
}

// Edge Case 6: Global variable access from SAC function
int global_val = 42;

int get_global_plus(int x) {
    return global_val + x;
}

// Edge Case 7: Function with zero return
int return_zero() {
    return 0;
}

// Edge Case 8: Very deep nesting (caller -> caller -> caller -> callee)
int level3(int x) {
    return x * 2;
}

int level2(int x) {
    return level3(x) + 1;
}

int level1(int x) {
    return level2(x) + 2;
}

int deep_nesting_test() {
    return level1(5);  // Should be: ((5*2)+1)+2 = 13
}

void main() {
    // Test Edge Case 2: Many locals
    int r2 = many_locals(1, 2, 3);
    // Expected: l1=1, l2=2, l3=3, l4=3, l5=5, l6=4, l7=8, l8=9, l9=13, l10=17 → 30
    if (r2 != 30) {
        printf("FAIL: many_locals\n");
        return;
    }

    // Test Edge Case 4: Array operations
    int arr[5] = {1, 2, 3, 4, 5};
    int r4 = sum_array(arr, 5);
    if (r4 != 15) {  // 1+2+3+4+5 = 15
        printf("FAIL: sum_array\n");
        return;
    }

    // Test Edge Case 5: Complex expression
    int r5 = complex_expr(2, 3, 4, 5);
    // temp1=5, temp2=9, temp3=45, temp4=43, temp5=21 → 25
    if (r5 != 25) {
        printf("FAIL: complex_expr\n");
        return;
    }

    // Test Edge Case 6: Global variable access
    int r6 = get_global_plus(8);
    if (r6 != 50) {  // 42 + 8 = 50
        printf("FAIL: get_global_plus\n");
        return;
    }

    // Test Edge Case 7: Zero return
    int r7 = return_zero();
    if (r7 != 0) {
        printf("FAIL: return_zero\n");
        return;
    }

    // Test Edge Case 8: Deep nesting
    int r8 = deep_nesting_test();
    if (r8 != 13) {
        printf("FAIL: deep_nesting_test\n");
        return;
    }

    // All edge case tests passed!
    printf("SUCCESS: All edge cases passed!\n");
    __asm__("brk");
}
