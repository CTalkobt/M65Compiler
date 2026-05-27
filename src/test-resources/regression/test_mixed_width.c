// test_mixed_width: char/int/long promotion and truncation
// Returns 0 (A=$00) on success, non-zero on failure.

int main() {
    char c;
    int i;
    long l;

    // Char to int promotion
    c = 42;
    i = c;
    if (i != 42) return 1;

    // Int to long promotion
    i = 30000;
    l = (long)i;
    if (l != 30000) return 2;

    // Long to int truncation
    l = 65538;
    i = (int)l;
    if (i != 2) return 3;  // low 16 bits

    // Int to char truncation
    i = 0x1234;
    c = (char)i;
    if (c != 0x34) return 4;

    // Char arithmetic stays 8-bit
    char a = 200;
    char b = 100;
    char r = a - b;
    if (r != 100) return 5;

    // Mixed char+int
    c = 10;
    i = 1000;
    int result = c + i;
    if (result != 1010) return 6;

    // Mixed int+long
    i = 5000;
    l = 100000;
    long lr = l + (long)i;
    if (lr != 105000) return 7;

    // Unsigned char
    unsigned char uc = 255;
    i = uc;
    if (i != 255) return 8;

    uc = uc + 1;
    if (uc != 0) return 9;  // wrap

    return 0;
}
