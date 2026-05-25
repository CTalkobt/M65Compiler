// Test compound assignment with mixed types (widening correctness)

// long += int
long test_long_plus_int(void) {
    long x = 1000;
    int y = 234;
    x += y;
    return x;  // 1234
}

// long -= int
long test_long_minus_int(void) {
    long x = 1000;
    int y = 234;
    x -= y;
    return x;  // 766
}

// long += char
long test_long_plus_char(void) {
    long x = 256;
    char c = 1;
    x += c;
    return x;  // 257
}

// int += char
int test_int_plus_char(void) {
    int x = 1000;
    char c = 200;
    x += c;
    return x;  // 1200
}

// int -= char
int test_int_minus_char(void) {
    int x = 1000;
    char c = 50;
    x -= c;
    return x;  // 950
}

// char += char (wraps)
int test_char_plus_char(void) {
    char a = 200;
    char b = 100;
    a += b;
    // 300 & 0xFF = 44
    return a;
}

// long &= int
long test_long_and_int(void) {
    long x = 255;   // 0x000000FF
    int mask = 15;   // 0x000F
    x &= mask;
    return x;  // 15
}

// long |= int
long test_long_or_int(void) {
    long x = 256;    // 0x00000100
    int bits = 15;   // 0x000F
    x |= bits;
    return x;  // 0x0000010F = 271
}

// int <<= char
int test_int_shl_char(void) {
    int x = 1;
    char n = 8;
    x <<= n;
    return x;  // 256
}

// int >>= char
int test_int_shr_char(void) {
    int x = 256;
    char n = 4;
    x >>= n;
    return x;  // 16
}

// Function call with long param + char param (rts #N correctness)
long add_long_char(long a, char b) {
    return a + b;
}

long add_long_int(long a, int b) {
    return a + b;
}

int main() {
    // long += int
    if (test_long_plus_int() != 1234) return 1;

    // long -= int
    if (test_long_minus_int() != 766) return 2;

    // long += char
    if (test_long_plus_char() != 257) return 3;

    // int += char
    if (test_int_plus_char() != 1200) return 4;

    // int -= char
    if (test_int_minus_char() != 950) return 5;

    // char += char (wraps)
    if (test_char_plus_char() != 44) return 6;

    // long &= int
    if (test_long_and_int() != 15) return 7;

    // long |= int
    if (test_long_or_int() != 271) return 8;

    // int <<= char
    if (test_int_shl_char() != 256) return 9;

    // int >>= char
    if (test_int_shr_char() != 16) return 10;

    // Cross-function long + char (rts #N fix)
    if (add_long_char(256, 1) != 257) return 11;
    if (add_long_char(1000, 234) != 1234) return 12;

    // Cross-function long + int
    if (add_long_int(256, 1) != 257) return 13;
    if (add_long_int(1000, 234) != 1234) return 14;

    return 0;
}
