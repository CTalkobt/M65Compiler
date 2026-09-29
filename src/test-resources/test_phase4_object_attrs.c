// Test: Phase 4 - Object file function attributes
// Validates: Leaf flags are stored in .o45 object files for inter-TU optimization

// Leaf function - should have FUNC_FLAG_LEAF in object file
int add(int a, int b) {
    return a + b;
}

// Leaf function - arithmetic only
int multiply(int x, int y) {
    return x * y;
}

// Leaf function - complex but no calls
int compute(int a, int b, int c) {
    return (a + b) * c - 10;
}

// Non-leaf function - calls add
int add_wrapper(int x, int y) {
    return add(x, y);
}

// Non-leaf function - calls multiply
int mul_wrapper(int x, int y) {
    return multiply(x, y);
}

// Non-leaf function - calls compute
int compute_wrapper(int a, int b, int c) {
    return compute(a, b, c);
}

// Non-leaf - calls other non-leaf functions
int chain_call(int a, int b, int c) {
    return add_wrapper(a, b) + compute_wrapper(a, b, c);
}

void main() {
    // These will be linked and the linker can use the leaf information
    // for call-site optimization (Phase 5)
    int r1 = add(1, 2);
    int r2 = multiply(3, 4);
    int r3 = compute(5, 6, 7);
    int r4 = chain_call(1, 2, 3);
}
