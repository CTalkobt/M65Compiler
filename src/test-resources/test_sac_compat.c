// SAC (Static Allocation Convention) regression test - backward compatibility
// Compile with: cc45 test_sac_compat.c -fstaticalloc -o test_sac_compat.prg
// Tests that SAC doesn't break non-SAC functions that might be in linked libraries

// These functions would normally come from libraries (non-SAC)
int lib_func(int a) {
    return a * 2;
}

// SAC function that uses library function
int sac_wrapper(int x) {
    return lib_func(x) + 10;
}

int main() {
    // Test 1: Direct library call
    int r1 = lib_func(5);
    if (r1 != 10) return 1;

    // Test 2: SAC wrapper calling library
    int r2 = sac_wrapper(5);
    if (r2 != 20) return 2;

    // Test 3: Multiple calls with different values
    int r3 = sac_wrapper(10);
    if (r3 != 30) return 3;

    // Test 4: Verify lib_func still works after wrapper
    int r4 = lib_func(100);
    if (r4 != 200) return 4;

    // All tests passed
    return 0;
}
