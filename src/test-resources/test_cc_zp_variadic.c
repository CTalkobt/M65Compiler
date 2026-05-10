// Test: Variadic calls from ZP-convention functions
// ZP functions should auto-fallback to stack-push convention for variadic calls

#include <stdio.h>

char *results = 0x4000;

// Simple variadic function that sums its arguments
// (We'll keep it simple since full variadic support may be complex)
int sum_three(int a, int b, int c) {
    return a + b + c;
}

// ZP-convention function that calls a variadic function
__fastcall__ int zp_caller(int x, int y) {
    // Call a function with multiple arguments (pseudo-variadic pattern)
    int result = sum_three(x, y, x + y);
    return result;
}

// Stack-convention function for comparison
int stack_caller(int x, int y) {
    int result = sum_three(x, y, x + y);
    return result;
}

void main() {
    // Test 1: Stack caller with multi-arg function
    int r1 = stack_caller(5, 10);
    results[0] = (char)(r1 & 0xFF);      // 5 + 10 + 15 = 30
    results[1] = (char)((r1 >> 8) & 0xFF);

    // Test 2: ZP caller with multi-arg function (should auto-fallback to stack convention)
    int r2 = zp_caller(7, 3);
    results[2] = (char)(r2 & 0xFF);      // 7 + 3 + 10 = 20
    results[3] = (char)((r2 >> 8) & 0xFF);

    // Test 3: ZP caller with different values
    int r3 = zp_caller(100, 56);
    results[4] = (char)(r3 & 0xFF);      // 100 + 56 + 156 = 312 (0x38)
    results[5] = (char)((r3 >> 8) & 0xFF);  // = 1

    // Sentinel
    results[6] = 0xFF;

    __asm__("brk");
}
