// SAC (Static Allocation Convention) - Phase 9 Emulator Execution Test
// Compile with: cc45 test_sac_execution.c -fstaticalloc -o test_sac_execution.prg
//
// This test verifies SAC functions produce correct results at runtime.
// Uses memory location 0x4000 as result marker:
//   0xAA = all tests passed
//   0xFF = test failed
//   Other = specific test failure code

volatile unsigned char *test_marker = (volatile unsigned char *)0x4000;

// Test 1: Basic arithmetic
int add(int a, int b) {
    return a + b;
}

// Test 2: Subtraction with negative result
int subtract(int a, int b) {
    return a - b;
}

// Test 3: Multiplication
int multiply(int a, int b) {
    return a * b;
}

// Test 4: Division
int divide(int a, int b) {
    if (b == 0) return -1;
    return a / b;
}

// Test 5: Function with local variables
int sum_with_locals(int x, int y) {
    int temp1 = x + 10;
    int temp2 = y + 20;
    return temp1 + temp2;
}

// Test 6: Nested calls
int nested_math(int a, int b, int c) {
    int sum = add(a, b);
    int prod = multiply(sum, c);
    return prod;
}

// Test 7: Multiple parameters
int five_param(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

// Test 8: No parameters
int get_magic_number() {
    return 42;
}

void main() {
    // Initialize result
    *test_marker = 0xFF;

    // Test 1: Add 10 + 20 = 30
    if (add(10, 20) != 30) {
        *test_marker = 0x01;
        __asm__("brk");
        return;
    }

    // Test 2: Subtract 50 - 30 = 20
    if (subtract(50, 30) != 20) {
        *test_marker = 0x02;
        __asm__("brk");
        return;
    }

    // Test 3: Multiply 6 * 7 = 42
    if (multiply(6, 7) != 42) {
        *test_marker = 0x03;
        __asm__("brk");
        return;
    }

    // Test 4: Divide 100 / 5 = 20
    if (divide(100, 5) != 20) {
        *test_marker = 0x04;
        __asm__("brk");
        return;
    }

    // Test 5: Locals - (5+10) + (15+20) = 50
    if (sum_with_locals(5, 15) != 50) {
        *test_marker = 0x05;
        __asm__("brk");
        return;
    }

    // Test 6: Nested - (5+10)*3 = 45
    if (nested_math(5, 10, 3) != 45) {
        *test_marker = 0x06;
        __asm__("brk");
        return;
    }

    // Test 7: Five params - 1+2+3+4+5 = 15
    if (five_param(1, 2, 3, 4, 5) != 15) {
        *test_marker = 0x07;
        __asm__("brk");
        return;
    }

    // Test 8: No params - get 42
    if (get_magic_number() != 42) {
        *test_marker = 0x08;
        __asm__("brk");
        return;
    }

    // All tests passed!
    *test_marker = 0xAA;
    __asm__("brk");
}
