// Phase 90 Test: Leaf Functions with Lazy FP
// Tests that leaf functions skip frame pointer setup

// Leaf function: no locals, no function calls
// Should skip FP setup (~15 byte savings)
int simple_leaf(int x, int y) {
    return x + y;
}

// Leaf function: simple arithmetic, no calls
int double_value(int x) {
    int result = x * 2;
    return result;
}

// Leaf function: multiple parameters, no calls
int add_three(int a, int b, int c) {
    return a + b + c;
}

// Leaf function: simple comparison
int max(int a, int b) {
    return a > b ? a : b;
}

// Leaf function: arithmetic only
long long_multiply(int x, long y) {
    return (long)x * y;
}

// NOT a leaf: has function call
int caller(int x) {
    return simple_leaf(x, x + 1);
}

// NOT a leaf: has locals
int with_locals(int x) {
    int temp = x * 2;
    int result = temp + 1;
    return result;
}

int main() {
    // Test leaf functions
    int r1 = simple_leaf(5, 3);
    int r2 = double_value(10);
    int r3 = add_three(1, 2, 3);
    int r4 = max(7, 4);
    long r5 = long_multiply(3, 4);

    // Test caller
    int r6 = caller(2);

    // Test with locals
    int r7 = with_locals(5);

    return r1 + r2 + r3 + r4 + (int)r5 + r6 + r7;
}
