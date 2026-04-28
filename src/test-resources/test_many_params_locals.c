// Test: Functions with many parameters (>2) and many local variables (>2)
// Validates frame-pointer-relative parameter access with deep stack frames.

int add4(int a, int b, int c, int d) {
    return a + b + c + d;
}

int add5(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

int weighted_sum(int a, int b, int c) {
    // Multiple locals alongside 3 params
    int w1 = a * 3;
    int w2 = b * 2;
    int w3 = c * 1;
    int total = w1 + w2 + w3;
    return total;
}

int many_locals(int p1, int p2) {
    // Many locals push FP offset far from SP
    int a = p1 + 1;
    int b = p2 + 2;
    int c = a + b;
    int d = c - p1;
    int e = d + p2;
    int f = a + e;
    return f;
}

int param_after_locals(int x, int y, int z) {
    // Declare locals, then access params — tests that FP offset tracks correctly
    int tmp1 = 0;
    int tmp2 = 0;
    int tmp3 = 0;
    tmp1 = x + 1;
    tmp2 = y + 2;
    tmp3 = z + 3;
    return tmp1 + tmp2 + tmp3;
}

int mixed_char_int(int a, char b, int c, char d) {
    int r = a + b + c + d;
    return r;
}

int call_chain(int a, int b, int c) {
    // Calls another multi-param function from within one
    int partial = add4(a, b, c, 10);
    int result = add4(partial, a, b, c);
    return result;
}

int deeply_nested(int a, int b, int c, int d) {
    // Local variables interleaved with param access
    int x = a + b;
    int y = c + d;
    int z = x + y;
    int w = z - a;
    int v = w + d;
    return v;
}

int main() {
    // Test 1: 4 parameters
    int r1 = add4(10, 20, 30, 40);
    if (r1 != 100) return 1;

    // Test 2: 5 parameters
    int r2 = add5(1, 2, 3, 4, 5);
    if (r2 != 15) return 2;

    // Test 3: 3 params + 4 locals
    int r3 = weighted_sum(10, 20, 30);
    if (r3 != 100) return 3;

    // Test 4: 2 params + 6 locals
    int r4 = many_locals(10, 5);
    if (r4 != 23) return 4;

    // Test 5: 3 params + 3 locals, params accessed after local declarations
    int r5 = param_after_locals(10, 20, 30);
    if (r5 != 66) return 5;

    // Test 6: Mixed char/int params
    int r6 = mixed_char_int(100, 10, 50, 5);
    if (r6 != 165) return 6;

    // Test 7: Calling multi-param functions from multi-param function
    int r7 = call_chain(1, 2, 3);
    if (r7 != 22) return 7;

    // Test 8: 4 params + 5 locals
    int r8 = deeply_nested(10, 20, 30, 40);
    if (r8 != 90) return 8;

    // Test 9: Passing locals as args to multi-param functions
    int a = 5;
    int b = 10;
    int c = 15;
    int d = 20;
    int r9 = add4(a, b, c, d);
    if (r9 != 50) return 9;

    // Test 10: Passing computed expressions as args
    int r10 = add5(a + b, b + c, c + d, d + a, a * 2);
    if (r10 != 110) return 10;

    return 0;
}
