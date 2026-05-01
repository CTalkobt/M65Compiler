// Test function pointers

int add(int a, int b) {
    return a + b;
}

int sub(int a, int b) {
    return a - b;
}

int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}

typedef int (*binop_t)(int, int);

int apply2(binop_t op, int x, int y) {
    return op(x, y);
}

int main(void) {
    // Basic function pointer declaration and call
    int (*fp)(int, int) = add;
    int r1 = fp(3, 4);

    // Typedef'd function pointer
    binop_t fp2 = sub;
    int r2 = fp2(10, 3);

    // Reassign function pointer
    fp2 = add;
    int r3 = fp2(10, 3);

    // Pass function pointer to function
    int r4 = apply(add, 5, 6);
    int r5 = apply(sub, 10, 3);

    // Typedef'd parameter
    int r6 = apply2(add, 8, 2);

    // Address-of syntax
    int (*fp3)(int, int) = &sub;
    int r7 = fp3(20, 5);

    // Dereference call syntax
    int r8 = (*fp)(2, 3);

    return r1 + r2 + r3 + r4 + r5 + r6 + r7 + r8;
}
