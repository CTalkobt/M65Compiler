// Phase 91.6: Threshold Tuning Benchmark
// Tests various function sizes and call patterns to tune inlining thresholds

// Small leaf function (3 bytes)
int tiny_leaf(void) {
    return 42;
}

// Small function with operation (8 bytes)
int small_leaf(int x) {
    return x + 1;
}

// Medium function (15 bytes)
int medium_func(int a, int b) {
    int result = a * 2 + b;
    return result;
}

// Larger function (25 bytes)
int large_func(int a, int b, int c) {
    int temp1 = a + b;
    int temp2 = temp1 * c;
    int result = temp2 - 5;
    return result;
}

// Single caller function (should inline if < threshold)
static int helper_single_caller(int x) {
    return x * x;
}

int call_helper_single(void) {
    return helper_single_caller(5) + helper_single_caller(3);
}

// Dead code function (should be eliminated)
static int dead_function(void) {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += i;
    }
    return sum;
}

// Constant parameter specialization candidate
int compute_distance(int x, int y, int z) {
    int x2 = x * x;
    int y2 = y * y;
    int z2 = z * z;
    int sum = x2 + y2 + z2;
    return sum;
}

int main(void) {
    // Use all functions to prevent dead code elimination
    int r1 = tiny_leaf();
    int r2 = small_leaf(10);
    int r3 = medium_func(5, 3);
    int r4 = large_func(2, 3, 4);
    int r5 = call_helper_single();
    int r6 = compute_distance(3, 4, 5);  // Common call with constant pattern
    int r7 = compute_distance(1, 1, 1);  // Another constant pattern

    // Return combined result to prevent optimization
    return r1 + r2 + r3 + r4 + r5 + r6 + r7;
}
