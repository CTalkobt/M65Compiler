// Test: Leaf function detection and .func_flags leaf directive emission
// Validates: Phase 3 fine-grained register invalidation (leaf detection)

volatile char *result = (char *)0x4000;

// Leaf function - no calls to other functions
int leaf_add(int a, int b) {
    return a + b;
}

// Leaf function - only arithmetic operations
int leaf_mul(int x, int y) {
    return x * y;
}

// Non-leaf function - calls leaf_add
int chain_add(int a, int b) {
    return leaf_add(a, b) + 10;
}

// Non-leaf function - calls chain_add
int chain_chain_add(int a, int b) {
    return chain_add(a, b) + 20;
}

// Leaf function - complex arithmetic but no calls
int leaf_complex(int a, int b, int c) {
    return (a + b) * c - 5;
}

void main() {
    // Test 1: Leaf function result
    int r1 = leaf_add(3, 4);  // Should be 7 = 0x07
    result[0] = r1;

    // Test 2: Leaf function with multiply
    int r2 = leaf_mul(5, 6);  // Should be 30 = 0x1E
    result[1] = r2;

    // Test 3: Non-leaf function result
    int r3 = chain_add(2, 3); // leaf_add(2,3) + 10 = 15 = 0x0F
    result[2] = r3;

    // Test 4: Chain of non-leaf calls
    int r4 = chain_chain_add(1, 2); // chain_add(1,2) + 20 = (leaf_add(1,2)+10) + 20 = 33 = 0x21
    result[3] = r4;

    // Test 5: Leaf function with complex arithmetic
    int r5 = leaf_complex(2, 3, 4); // (2+3)*4 - 5 = 15 = 0x0F
    result[4] = r5;

    // Sentinel
    result[5] = 0xAA;
}
