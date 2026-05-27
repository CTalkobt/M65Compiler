// test_inline: validate inline function expansion
// Returns 0 (A=$00) on success, non-zero on failure.

inline int add(int a, int b) {
    return a + b;
}

inline int square(int x) {
    return x * x;
}

inline int identity(int x) {
    return x;
}

inline void noop(void) {
    return;
}

int main() {
    // Basic inline call
    if (add(3, 4) != 7) return 1;

    // Inline with expressions
    if (add(10, 20) != 30) return 2;

    // Nested inline calls
    if (add(square(3), 1) != 10) return 3;

    // Identity
    if (identity(42) != 42) return 4;

    // Void inline
    noop();

    // Multiple uses
    int x = add(1, 2);
    int y = add(3, 4);
    if (x + y != 10) return 5;

    return 0;
}
