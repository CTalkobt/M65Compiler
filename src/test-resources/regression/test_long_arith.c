// test_long_arith: 32-bit long arithmetic regression tests
// Exercises AXYZ quad register operations on 45GS02.
// Returns 0 (A=$00) on success, non-zero on failure.

int main() {
    long a;
    long b;
    long r;

    // Addition
    a = 100000;
    b = 200000;
    r = a + b;
    if (r != 300000) return 1;

    // Subtraction
    r = b - a;
    if (r != 100000) return 2;

    // Negative result
    r = a - b;
    if (r != -100000) return 3;

    // Multiplication
    a = 1000;
    b = 1000;
    r = a * b;
    if (r != 1000000) return 4;

    // Division
    a = 1000000;
    b = 1000;
    r = a / b;
    if (r != 1000) return 5;

    // Modulo
    a = 1000007;
    b = 1000;
    r = a % b;
    if (r != 7) return 6;

    // Comparison
    a = 50000;
    b = 50000;
    if (a != b) return 7;
    if (a < b) return 8;
    if (a > b) return 9;

    a = 50000;
    b = 50001;
    if (!(a < b)) return 10;
    if (a >= b) return 11;

    // Shift left
    a = 1;
    r = a << 16;
    if (r != 65536) return 12;

    // Shift right
    a = 65536;
    r = a >> 8;
    if (r != 256) return 13;

    // Bitwise
    a = 0xFF00FF;
    b = 0x0F0F0F;
    r = a & b;
    if (r != 0x0F000F) return 14;

    r = a | b;
    if (r != 0xFF0FFF) return 15;

    return 0;
}
