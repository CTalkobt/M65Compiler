// Test: Phase 5 - Inter-TU optimization (external object file clobber info)
// Validates: AssemblerOptimizer loads and uses clobber info from .o45 files

// Leaf function - no calls, minimal clobber
int leaf_compute(int x) {
    return x * 2 + 5;
}

// Leaf function - multiple operations but no calls
int leaf_combine(int a, int b, int c) {
    return (a + b) * c - 10;
}

// Non-leaf function - calls leaf functions
int chain_compute(int x, int y) {
    return leaf_compute(x) + leaf_compute(y);
}

// Non-leaf function - multiple calls
int chain_combine(int a, int b, int c) {
    int temp = leaf_combine(a, b, c);
    return leaf_compute(temp);
}

void main() {
    // Test 1: Direct leaf call (should use leaf clobber info in Phase 5)
    int r1 = leaf_compute(10);  // 10*2+5 = 25 = 0x19

    // Test 2: Multiple parameter leaf call
    int r2 = leaf_combine(2, 3, 4);  // (2+3)*4-10 = 10 = 0x0A

    // Test 3: Non-leaf calling leaf
    int r3 = chain_compute(5, 7);  // leaf_compute(5) + leaf_compute(7) = 15 + 19 = 34 = 0x22

    // Test 4: Complex chain
    int r4 = chain_combine(1, 2, 3);  // leaf_combine(1,2,3) = (1+2)*3-10 = -1 -> unsigned wrap
}
