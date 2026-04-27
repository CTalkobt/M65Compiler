// Test: ZP temp registers not clobbered by function calls
// Validates the fix for array stores where the RHS is a function call.

int double_it(int x) {
    return x * 2;
}

int add(int a, int b) {
    return a + b;
}

int main() {
    int results[4];

    // Simple: array store with function call RHS
    results[0] = double_it(5);

    // Function call in index position (emitAddress involves eval)
    int idx = 1;
    results[idx] = double_it(3);

    // Nested function calls in expression
    results[2] = add(double_it(2), 1);

    // Compound assignment with function call RHS
    results[3] = 10;
    results[3] += double_it(5);

    // Verify: results[0]=10, results[1]=6, results[2]=5, results[3]=20
    if (results[0] != 10) return 1;
    if (results[1] != 6) return 2;
    if (results[2] != 5) return 3;
    if (results[3] != 20) return 4;
    return 0;
}
