// SAC (Static Allocation Convention) regression test - pragma override
// Compile with: cc45 test_sac_pragma.c -fstaticalloc -o test_sac_pragma.prg
// Tests that #pragma cc45 no_static_alloc can disable SAC per-function

#pragma cc45 no_static_alloc
int non_sac_add(int a, int b) {
    return a + b;
}

// Re-enable SAC for subsequent functions (or -fstaticalloc applies)
int sac_multiply(int a, int b) {
    return a * b;
}

int main() {
    // Test 1: non_sac_add function
    int r1 = non_sac_add(10, 20);
    if (r1 != 30) return 1;

    // Test 2: sac_multiply function
    int r2 = sac_multiply(5, 6);
    if (r2 != 30) return 2;

    // Test 3: Mix of both conventions
    int r3 = non_sac_add(r2, 70);
    if (r3 != 100) return 3;

    // All tests passed
    return 0;
}
