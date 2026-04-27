// Test: String literal storage and byte-by-byte access
// Validates that string literals are stored correctly and accessible via pointer.

int main() {
    char *s = "HELLO";

    // Verify each byte
    if (s[0] != 72) return 1;   // 'H' = 0x48 = 72
    if (s[1] != 69) return 2;   // 'E' = 0x45 = 69
    if (s[2] != 76) return 3;   // 'L' = 0x4C = 76
    if (s[3] != 76) return 4;   // 'L' = 0x4C = 76
    if (s[4] != 79) return 5;   // 'O' = 0x4F = 79
    if (s[5] != 0) return 6;    // null terminator

    // String length via loop
    int len = 0;
    while (s[len] != 0) {
        len = len + 1;
    }
    if (len != 5) return 7;

    // Two string literals should be independently accessible
    char *t = "AB";
    if (t[0] != 65) return 8;   // 'A' = 0x41 = 65
    if (t[1] != 66) return 9;   // 'B' = 0x42 = 66
    if (t[2] != 0) return 10;   // null terminator

    // Original string still valid
    if (s[0] != 72) return 11;

    return 0;
}
