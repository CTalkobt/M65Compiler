// Phase 108.4: Simple Validation Test
// Focuses on hook invocation and decision logic

// Test 1: Constant folding
int test_constant_folding() {
    int a = 10;
    int b = 20;
    int c = a + b;
    return c;
}

// Test 2: Common subexpression elimination
int test_cse() {
    int x = 100;
    int y = x + 50;
    int z = x + 50;
    return y + z;
}

// Test 3: Dead code elimination
int test_dead_code() {
    int unused = 999;
    int used = 123;
    return used;
}

// Test 4: Inlining candidate
static int inline_func(int x) {
    return x + 1;
}

int test_inlining() {
    return inline_func(5) + inline_func(10);
}

// Test 5: Loop optimization
int test_loop() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    return sum;
}

// Test 6: Function calls for hook tracking
int helper1() { return 1; }
int helper2() { return 2; }
int helper3() { return 3; }
int helper4() { return 4; }
int helper5() { return 5; }

int test_function_analysis() {
    return helper1() + helper2() + helper3() + helper4() + helper5();
}

// Test 7: Array access
int test_array() {
    int arr[5] = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += arr[i];
    }
    return sum;
}

// Test 8: Branch density
int test_branches(int x) {
    if (x > 0) {
        if (x > 5) {
            return x * 2;
        }
        return x;
    }
    return 0;
}

// Test 9: Arithmetic optimization
int test_arithmetic() {
    int x = 10;
    int y = x * 4;  // Should become x << 2
    int z = x / 2;  // Should become x >> 1
    return y + z;
}

// Test 10: Variable scope
int test_scope() {
    int a = 10;
    {
        int b = 20;
        a = a + b;
    }
    return a;
}

int main() {
    int result = 0;
    result += test_constant_folding();
    result += test_cse();
    result += test_dead_code();
    result += test_inlining();
    result += test_loop();
    result += test_function_analysis();
    result += test_array();
    result += test_branches(3);
    result += test_arithmetic();
    result += test_scope();
    return result;
}
