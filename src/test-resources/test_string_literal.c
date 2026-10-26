// Test: String literal storage and byte-by-byte access
// Validates that string literals are stored correctly and accessible via pointer.

int main() {
    char *s = "HELLO";

    // Verify each byte (literals are PETSCII: 'H' is $C8)
    if (s[0] != 'H') return 1;
    if (s[1] != 'E') return 2;
    if (s[2] != 'L') return 3;
    if (s[3] != 'L') return 4;
    if (s[4] != 'O') return 5;
    if (s[5] != 0) return 6;    // null terminator

    // String length via loop
    int len = 0;
    while (s[len] != 0) {
        len = len + 1;
    }
    if (len != 5) return 7;

    // Two string literals should be independently accessible
    char *t = "AB";
    if (t[0] != 'A') return 8;
    if (t[1] != 'B') return 9;
    if (t[2] != 0) return 10;   // null terminator

    // Original string still valid
    if (s[0] != 'H') return 11;

    return 0;
}
