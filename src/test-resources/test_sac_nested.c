// SAC (Static Allocation Convention) regression test - nested function calls
// Compile with: cc45 test_sac_nested.c -fstaticalloc -o test_sac_nested.prg
// Tests SAC with nested function calls and parameter preservation

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int compute(int x, int y, int z) {
    // Nested calls: compute uses add and subtract
    int sum = add(x, y);
    int diff = subtract(z, sum);
    return diff;
}

int main() {
    // Test 1: Direct nested call
    // compute(5, 10, 30) = subtract(30, add(5, 10)) = subtract(30, 15) = 15
    int r1 = compute(5, 10, 30);
    if (r1 != 15) return 1;

    // Test 2: Another nested call with different values
    // compute(2, 3, 20) = subtract(20, add(2, 3)) = subtract(20, 5) = 15
    int r2 = compute(2, 3, 20);
    if (r2 != 15) return 2;

    // Test 3: Verify add still works independently after compute
    int r3 = add(100, 200);
    if (r3 != 300) return 3;

    // All tests passed
    return 0;
}
