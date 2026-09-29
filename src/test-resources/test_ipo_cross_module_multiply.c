// Phase 92 IPO: Cross-Module Function Specialization
// Tests: Two modules calling same function with same constant pattern
// Expected: Single specialized variant shared across modules

// helper_multiply.c (simulated)
int multiply_by_n(int x, int n) {
    return x * n;
}

// main module
int test_multiply_by_4(void) {
    // Call with constant n=4
    return multiply_by_n(10, 4);  // 10 * 4 = 40
}

int test_multiply_by_8(void) {
    // Call with constant n=8 (different pattern)
    return multiply_by_n(5, 8);   // 5 * 8 = 40
}

int test_multiply_generic(int x, int n) {
    // Call with non-constant n
    return multiply_by_n(x, n);
}

int main(void) {
    int r1 = test_multiply_by_4();   // Should use specialized variant n=4
    int r2 = test_multiply_by_8();   // Should use specialized variant n=8
    int r3 = test_multiply_generic(3, 7);  // Should use generic variant

    if (r1 != 40) return 1;
    if (r2 != 40) return 2;
    if (r3 != 21) return 3;
    return 0;
}
