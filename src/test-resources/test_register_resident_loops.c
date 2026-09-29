// Test case for RegisterResidentLoops optimization
// Phase C4.1: Loop counter X-register allocation
//
// Expected behavior:
// - Simple 8-bit loop counters should be optimized for X register
// - Code size should be reduced by 5-10% for loop-heavy code
// - No behavioral change (results must be identical)

// Test 1: Simple increment loop (i++)
int test_simple_increment() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    return sum;  // Should return 45 (0+1+2+...+9)
}

// Test 2: Simple decrement loop (i--)
int test_simple_decrement() {
    int sum = 0;
    for (int i = 10; i > 0; i--) {
        sum += i;
    }
    return sum;  // Should return 55 (10+9+8+...+1)
}

// Test 3: Loop with comparison (i <= bound)
int test_loop_le() {
    int product = 1;
    for (int i = 1; i <= 5; i++) {
        product *= i;
    }
    return product;  // Should return 120 (5! = 120)
}

// Test 4: Nested loops (both should be candidates)
int test_nested_loops() {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            sum++;
        }
    }
    return sum;  // Should return 25
}

// Test 5: Loop with body (verify no conflicts)
int test_loop_with_ops() {
    int result = 0;
    for (int i = 0; i < 8; i++) {
        result += (i << 2);  // Shift operations
    }
    return result;  // Should return 0+4+8+12+16+20+24+28 = 112
}

// Test 6: Counter used in array indexing
int test_array_loop() {
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += arr[i];
    }
    return sum;  // Should return 45
}

// Test 7: Loop with multiple operations
int test_complex_body() {
    int x = 0, y = 0;
    for (int i = 0; i < 256; i++) {
        x += i;
        y -= (i >> 1);
    }
    return x + y;
}

// Test 8: Counter of different type (should also be candidate)
unsigned char test_uchar_loop() {
    unsigned char sum = 0;
    for (unsigned char i = 0; i < 255; i++) {
        sum += i;
    }
    return sum;
}

int main() {
    // Run all tests and verify results
    if (test_simple_increment() != 45) return -1;
    if (test_simple_decrement() != 55) return -2;
    if (test_loop_le() != 120) return -3;
    if (test_nested_loops() != 25) return -4;
    if (test_loop_with_ops() != 112) return -5;
    if (test_array_loop() != 45) return -6;
    if (test_uchar_loop() != 254) return -7;  // Note: 0-254 sum = 254

    return 0;  // All tests passed
}
