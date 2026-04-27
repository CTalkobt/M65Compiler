// Test: Function argument passing including phw.sp
// Validates 16-bit stack-relative variable push for function calls.

int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

int three_args(int a, int b, int c) {
    return a + b + c;
}

int nested_call(int x) {
    return add(x, x);
}

int main() {
    int a = 100;
    int b = 50;

    // Basic two-arg call with local variables
    int r1 = add(a, b);
    if (r1 != 150) return 1;

    // Subtraction with locals
    int r2 = sub(a, b);
    if (r2 != 50) return 2;

    // Three arguments
    int c = 25;
    int r3 = three_args(a, b, c);
    if (r3 != 175) return 3;

    // Passing expression results
    int r4 = add(a + 10, b - 10);
    if (r4 != 150) return 4;

    // Nested calls: add(100, 100) = 200
    int r5 = nested_call(a);
    if (r5 != 200) return 5;

    // Result used in further computation
    int r6 = add(r1, r2);
    if (r6 != 200) return 6;

    return 0;
}
