// Phase 92 IPO: Cross-Module Specialization with Mixed Patterns
// Tests: Multiple specialization patterns across different functions

int add_n(int x, int n) {
    return x + n;
}

int scale_by(int x, int factor) {
    return x * factor;
}

int shift_left(int x, int bits) {
    return x << bits;
}

// Pattern 1: add_n with constant 10
int helper_add_10(int x) {
    return add_n(x, 10);
}

// Pattern 2: scale_by with constant 4
int helper_scale_4(int x) {
    return scale_by(x, 4);
}

// Pattern 3: shift_left with constant 3
int helper_shift_3(int x) {
    return shift_left(x, 3);
}

// Generic patterns
int helper_add_generic(int x, int n) {
    return add_n(x, n);
}

int helper_scale_generic(int x, int factor) {
    return scale_by(x, factor);
}

int main(void) {
    // Test specialized patterns
    if (helper_add_10(5) != 15) return 1;      // 5 + 10
    if (helper_scale_4(3) != 12) return 2;    // 3 * 4
    if (helper_shift_3(2) != 16) return 3;    // 2 << 3 = 16

    // Test generic patterns
    if (helper_add_generic(5, 7) != 12) return 4;    // 5 + 7
    if (helper_scale_generic(3, 6) != 18) return 5;  // 3 * 6

    return 0;
}
