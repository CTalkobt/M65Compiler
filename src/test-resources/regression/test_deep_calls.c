// test_deep_calls: deep call chains and stack-intensive patterns
// Returns 0 (A=$00) on success, non-zero on failure.

int add1(int x) { return x + 1; }
int add2(int x) { return add1(add1(x)); }
int add4(int x) { return add2(add2(x)); }
int add8(int x) { return add4(add4(x)); }

int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int many_locals(int a) {
    int v0 = a;
    int v1 = v0 + 1;
    int v2 = v1 + 1;
    int v3 = v2 + 1;
    int v4 = v3 + 1;
    int v5 = v4 + 1;
    int v6 = v5 + 1;
    int v7 = v6 + 1;
    return v0 + v1 + v2 + v3 + v4 + v5 + v6 + v7;
}

int main() {
    // Deep call chain
    if (add8(0) != 8) return 1;
    if (add8(10) != 18) return 2;

    // Recursion
    if (fibonacci(0) != 0) return 3;
    if (fibonacci(1) != 1) return 4;
    if (fibonacci(6) != 8) return 5;
    if (fibonacci(10) != 55) return 6;

    // Many locals
    // v0=1, v1=2, ..., v7=8 → sum = 1+2+3+4+5+6+7+8 = 36
    if (many_locals(1) != 36) return 7;

    // Nested expressions
    int r = add4(add4(add4(0)));
    if (r != 12) return 8;

    return 0;
}
