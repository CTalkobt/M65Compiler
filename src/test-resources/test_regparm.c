// Test __regparm function attribute (first param in A/AX register)

__regparm char char_double(char x) { return x + x; }
__regparm int int_add_ten(int x) { return x + 10; }
__regparm char char_identity(char x) { return x; }

// Multi-param: first in A, rest on stack/ZP
__regparm int add(int a, int b) { return a + b; }

int main() {
    // Char param in A
    if (char_double(21) != 42) return 1;
    if (char_double(0) != 0) return 2;
    if (char_double(127) != 254) return 3;

    // Int param in AX
    if (int_add_ten(32) != 42) return 4;
    if (int_add_ten(0) != 10) return 5;
    if (int_add_ten(1000) != 1010) return 6;

    // Identity (value preserved through register)
    if (char_identity(255) != 255) return 7;
    if (char_identity(0) != 0) return 8;

    // Multi-param
    if (add(10, 20) != 30) return 9;
    if (add(100, 200) != 300) return 10;

    return 0;
}
