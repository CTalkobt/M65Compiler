/**
 * Phase 91.1: Global Function Database Test
 *
 * Tests basic function profile collection and database operations.
 * Verifies that the GlobalFunctionDatabase correctly tracks functions
 * and identifies specialization candidates.
 */

// Test Case 1: Leaf functions (no locals, no calls)
int leaf_abs(int x) {
    return (x < 0) ? -x : x;
}

int leaf_max(int a, int b) {
    return (a > b) ? a : b;
}

int leaf_min(int a, int b) {
    return (a < b) ? a : b;
}

// Test Case 2: Functions with constant argument patterns
int process_flags(int flags) {
    if (flags & 0x01) return 1;
    if (flags & 0x02) return 2;
    if (flags & 0x04) return 4;
    return 0;
}

int convert_type(int type) {
    switch (type) {
        case 1: return 10;
        case 2: return 20;
        case 3: return 30;
        default: return 0;
    }
}

// Test Case 3: Functions with both constant and variable calls
int handler_event(int eventType, int data) {
    if (eventType == 1) {
        return data * 2;
    }
    return data + 1;
}

// Test Case 4: Caller tracking
void main_processor(void) {
    int a = leaf_abs(-42);
    int b = leaf_max(10, 20);
    int c = leaf_min(5, 15);

    // Constant argument calls (specialization candidates)
    int f1 = process_flags(0x01);
    int f2 = process_flags(0x02);
    int f3 = process_flags(0x01);  // Same as f1

    // Variable argument calls
    int e1 = handler_event(1, a);
    int e2 = handler_event(2, b);
}

// Test Case 5: Functions never called (dead code)
int unused_function(int x) {
    return x * 100;
}

int main(void) {
    main_processor();
    return 0;
}
