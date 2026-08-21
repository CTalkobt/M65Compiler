// Phase 108: Frontend Integration Hook Testing
// Tests hook infrastructure and signal collection

// Test 1: Simple function with constant folding
int test_folding() {
    int a = 10;
    int b = 20;
    int c = a + b;  // Should be folded to 30
    return c;
}

// Test 2: Function with inlining candidate
int inline_candidate(int x) {
    return x + 1;
}

int test_inlining() {
    return inline_candidate(5) + inline_candidate(10);
}

// Test 3: Complex function triggering IR optimization
int test_ir_opt(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum = sum + i * 2;  // Loop optimization candidate
    }
    return sum;
}

// Test 4: Function with call graph (multiple calls)
int recursive_test(int n) {
    if (n <= 0) return 1;
    return n + recursive_test(n - 1);
}

// Test 5: Assembly generation test
void test_codegen() {
    volatile unsigned char* screen = (volatile unsigned char*)0x0400;
    for (int i = 0; i < 40; i++) {
        screen[i] = 0x41;  // Fill with 'A'
    }
}

// Main entry point
int main() {
    int result = 0;
    result += test_folding();
    result += test_inlining();
    result += test_ir_opt(10);
    result += recursive_test(5);
    test_codegen();
    return result;
}
