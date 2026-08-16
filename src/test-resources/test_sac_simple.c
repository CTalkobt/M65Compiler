// SAC (Static Allocation Convention) regression test - simple function calls
// Compile with: cc45 test_sac_simple.c -fstaticalloc -o test_sac_simple.prg
// Tests basic SAC functionality: parameter passing, calculation, return values

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    // Test 1: Simple addition
    int r1 = add(10, 20);
    if (r1 != 30) return 1;

    // Test 2: Multiplication
    int r2 = multiply(5, 6);
    if (r2 != 30) return 2;

    // Test 3: Chained calls
    int r3 = add(r1, r2);
    if (r3 != 60) return 3;

    // All tests passed
    return 0;
}
