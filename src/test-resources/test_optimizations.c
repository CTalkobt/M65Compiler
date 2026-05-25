// Test correctness of codegen optimizations
// Verifies that optimizations don't break program semantics

// --- Pointer constant propagation ---
int test_ptr_const_prop(void) {
    unsigned char *p = (unsigned char *)0x4000;
    *p = 42;
    *p = 99;
    // Read back to verify
    unsigned char val = *(unsigned char *)0x4000;
    if (val != 99) return 1;
    return 0;
}

// --- Fused compare-and-branch ---
int test_fused_cmp(int a, int b) {
    if (a < b) return 1;
    if (a > b) return 2;
    if (a == b) return 3;
    return 0;
}

// --- AND/OR branch fusion ---
int test_and_branch(unsigned char flags) {
    if (flags & 1) return 1;
    if (flags & 2) return 2;
    if (flags & 4) return 4;
    if (flags & 128) return 128;
    return 0;
}

int test_or_branch(unsigned char a, unsigned char b) {
    if (a | b) return 1;
    return 0;
}

// --- Dead local elimination (must NOT eliminate used variables) ---
int test_dead_local_keeps_used(void) {
    int used = 42;
    int unused = 99;  // should be eliminated
    return used;
}

// --- while(1) with break ---
int test_while1_break(void) {
    int count = 0;
    while (1) {
        count++;
        if (count == 5) break;
    }
    return count;
}

// --- Void call cleanup ---
void void_func(void) { }

int test_void_call(void) {
    void_func();
    return 42;
}

// --- Return constant ---
int return_zero(void) { return 0; }
int return_42(void) { return 42; }
char return_char(void) { return 255; }

// --- Branch inversion correctness ---
int test_branch_inversion(int x) {
    int result = 0;
    if (x < 10) {
        result = 1;
    } else {
        result = 2;
    }
    return result;
}

// --- Compound assignment with different types ---
int test_compound_assign(void) {
    int x = 100;
    x += 50;
    x -= 25;
    x *= 2;
    return x;  // (100 + 50 - 25) * 2 = 250
}

// --- Char increment/decrement wrap ---
int test_char_wrap(void) {
    unsigned char c = 255;
    c++;
    if (c != 0) return 1;
    c--;
    if (c != 255) return 2;
    return 0;
}

// --- I8 comparison with literal ---
int test_i8_cmp(unsigned char val) {
    if (val < 10) return 1;
    if (val > 200) return 2;
    if (val == 100) return 3;
    return 0;
}

int main() {
    // Pointer constant propagation
    if (test_ptr_const_prop() != 0) return 1;

    // Fused compare-and-branch
    if (test_fused_cmp(5, 10) != 1) return 2;
    if (test_fused_cmp(10, 5) != 2) return 3;
    if (test_fused_cmp(7, 7) != 3) return 4;

    // AND/OR branch fusion
    if (test_and_branch(1) != 1) return 5;
    if (test_and_branch(2) != 2) return 6;
    if (test_and_branch(4) != 4) return 7;
    if (test_and_branch(128) != 128) return 8;
    if (test_and_branch(0) != 0) return 9;
    if (test_or_branch(0, 0) != 0) return 10;
    if (test_or_branch(1, 0) != 1) return 11;
    if (test_or_branch(0, 1) != 1) return 12;

    // Dead local keeps used variables
    if (test_dead_local_keeps_used() != 42) return 13;

    // while(1) with break
    if (test_while1_break() != 5) return 14;

    // Void call cleanup
    if (test_void_call() != 42) return 15;

    // Return constants
    if (return_zero() != 0) return 16;
    if (return_42() != 42) return 17;
    if (return_char() != 255) return 18;

    // Branch inversion
    if (test_branch_inversion(5) != 1) return 19;
    if (test_branch_inversion(15) != 2) return 20;

    // Compound assignment
    if (test_compound_assign() != 250) return 21;

    // Char wrap
    if (test_char_wrap() != 0) return 22;

    // I8 comparison
    if (test_i8_cmp(5) != 1) return 23;
    if (test_i8_cmp(250) != 2) return 24;
    if (test_i8_cmp(100) != 3) return 25;
    if (test_i8_cmp(50) != 0) return 26;

    return 0;
}
