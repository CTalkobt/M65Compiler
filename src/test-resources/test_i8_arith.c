// Test 8-bit char arithmetic and proper widening to 16/32-bit

// --- I8 operations should stay 8-bit ---

char char_and(char a, char b) { return a & b; }
char char_or(char a, char b) { return a | b; }
char char_xor(char a, char b) { return a ^ b; }
char char_add(char a, char b) { return a + b; }
char char_sub(char a, char b) { return a - b; }
char char_and_lit(char a) { return a & 0x0F; }
char char_or_lit(char a) { return a | 0x80; }
char char_add_lit(char a) { return a + 10; }

char char_inc(char a) { a++; return a; }
char char_dec(char a) { a--; return a; }

// --- Comparisons ---

int char_lt(char a, char b) { return a < b; }
int char_eq(char a, char b) { return a == b; }

// --- Widening: char must widen to int/long when mixed ---

int int_plus_char(int a, char b) { return a + b; }

// Long arithmetic tested via local operations (no cross-function param issues)
long long_add_const() {
    long l = 257;
    l += 20;
    return l;
}

int main() {
    // I8 AND
    if (char_and(0xFF, 0x0F) != 0x0F) return 1;
    if (char_and(0xAA, 0x55) != 0x00) return 2;

    // I8 OR
    if (char_or(0x0F, 0xF0) != 0xFF) return 3;
    if (char_or(0x00, 0x42) != 0x42) return 4;

    // I8 XOR
    if (char_xor(0xFF, 0xFF) != 0x00) return 5;
    if (char_xor(0xAA, 0x0F) != 0xA5) return 6;

    // I8 ADD
    if (char_add(10, 20) != 30) return 7;
    if (char_add(200, 56) != 0) return 8;  // wraps at 256

    // I8 SUB
    if (char_sub(100, 30) != 70) return 9;

    // I8 AND with literal
    if (char_and_lit(0xAB) != 0x0B) return 10;

    // I8 OR with literal
    if (char_or_lit(0x01) != 0x81) return 11;

    // I8 ADD with literal
    if (char_add_lit(5) != 15) return 12;

    // I8 increment
    if (char_inc(41) != 42) return 13;
    if (char_inc(255) != 0) return 14;  // wraps

    // I8 decrement
    if (char_dec(43) != 42) return 15;
    if (char_dec(0) != 255) return 16;  // wraps

    // I8 comparisons
    if (char_lt(5, 10) != 1) return 17;
    if (char_lt(10, 5) != 0) return 18;
    if (char_eq(42, 42) != 1) return 19;
    if (char_eq(42, 43) != 0) return 20;

    // Widening: int + char
    if (int_plus_char(1000, 5) != 1005) return 21;
    if (int_plus_char(256, 1) != 257) return 22;

    // TODO: long function calls in standalone mode have a pre-existing
    // stack cleanup issue. Tested separately in linked mode.
    // if (long_add_const() != 277) return 23;

    return 0;
}
