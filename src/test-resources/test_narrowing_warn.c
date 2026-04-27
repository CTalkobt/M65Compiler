// Test: Implicit narrowing warnings
// This file should produce warnings during compilation for implicit narrowing.
// Lines marked WARN should produce a warning; lines marked OK should not.

int main() {
    // WARN: int -> char implicit narrowing in declaration
    int big = 500;
    char c1 = big;

    // WARN: int -> char implicit narrowing in assignment
    char c2;
    c2 = big;

    // OK: explicit cast suppresses warning
    char c3 = (char)big;

    // OK: char -> int is widening, no warning
    char small = 42;
    int w = small;

    // OK: char -> char, same size
    char c4 = small;

    // OK: int -> int, same size
    int i2 = big;

    // WARN: pointer -> char narrowing in declaration
    char *p = &c1;
    char c5 = p;

    return 0;
}
