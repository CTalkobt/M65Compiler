/**
 * Phase 91.2: IPO Analysis Test
 *
 * Tests global analysis engine for identifying specialization, inlining,
 * and dead code elimination opportunities across function boundaries.
 */

// Test Case 1: Single-caller function (inlining candidate)
int get_constant(void) {
    return 42;
}

// Test Case 2: Tiny leaf function (inlining candidate)
int abs_tiny(int x) {
    return (x < 0) ? -x : x;
}

// Test Case 3: Small function with constant-only calls (specialization candidate)
int dispatch_event(int type) {
    if (type == 1) return 100;
    if (type == 2) return 200;
    return 0;
}

// Test Case 4: Larger function with mixed calls (no inlining)
int process_data(int* arr, int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += arr[i];
        sum = dispatch_event(1);  // Constant call
    }
    return sum;
}

// Test Case 5: Dead code (never called)
int unused_handler(int x) {
    return x * 1000;
}

// Test Case 6: Another dead code
int never_called_func(void) {
    return 999;
}

// Main function using some of the above
int main(void) {
    int v1 = get_constant();      // Inlining candidate: single caller
    int v2 = abs_tiny(42);        // Inlining candidate: tiny
    int v3 = dispatch_event(1);   // Specialization candidate: constant args
    int v4 = dispatch_event(2);   // Same pattern
    int v5 = process_data(0, 0);  // Regular call

    return v1 + v2 + v3 + v4 + v5;
}
