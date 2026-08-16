// SAC Performance Benchmark - Baseline (non-SAC version)
// Compile WITHOUT -fstaticalloc: cc45 test_sac_perf_baseline.c -o baseline.prg
// Compare binary size and cycles with SAC version

// Test 1: Simple arithmetic function
int add_nums(int a, int b) {
    return a + b;
}

// Test 2: Function with local variables
int calculate(int x, int y, int z) {
    int temp1 = x + y;
    int temp2 = y + z;
    int result = temp1 + temp2;
    return result;
}

// Test 3: Nested function calls
int complex_calc(int a, int b, int c, int d) {
    int sum1 = add_nums(a, b);
    int sum2 = add_nums(c, d);
    return sum1 + sum2;
}

// Test 4: Loop with function calls
int sum_via_calls(int n) {
    int total = 0;
    int i = 0;
    while (i < n) {
        total = add_nums(total, i);
        i = add_nums(i, 1);
    }
    return total;
}

// Test 5: Five parameters
int five_add(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

void main() {
    // Run through all benchmark functions
    int r1 = add_nums(10, 20);
    int r2 = calculate(5, 10, 15);
    int r3 = complex_calc(1, 2, 3, 4);
    int r4 = sum_via_calls(5);
    int r5 = five_add(1, 2, 3, 4, 5);

    // Exit via BRK
    __asm__("brk");
}
