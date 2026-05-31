// test_strncpy: validate strncpy() - limited-length string copying
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char src[16] = "hello world";
    char dst[16];
    int i;

    // Test 1: Copy exact length
    strncpy(dst, src, 5);
    dst[5] = '\0'; // Manual null termination
    if (strcmp(dst, "hello") != 0) return 1;

    // Test 2: Copy with limit larger than string
    for (i = 0; i < 16; i++) dst[i] = 0;
    strncpy(dst, src, 20);
    if (strcmp(dst, "hello world") != 0) return 2;

    // Test 3: Copy zero bytes
    dst[0] = 'X';
    strncpy(dst, src, 0);
    if (dst[0] != 'X') return 3; // Should not copy

    // Test 4: Copy partial string
    for (i = 0; i < 16; i++) dst[i] = 0;
    strncpy(dst, "testing", 4);
    if (dst[0] != 't' || dst[1] != 'e' || dst[2] != 's' || dst[3] != 't') return 4;
    if (dst[4] != '\0') return 5; // Should null-terminate at limit

    // Test 5: Copy single character
    for (i = 0; i < 16; i++) dst[i] = 0;
    strncpy(dst, "abc", 1);
    if (dst[0] != 'a' || dst[1] != '\0') return 6;

    // Test 6: Copy to overlapping buffer (forward)
    char overlap[16] = "abcdefgh";
    strncpy(overlap + 2, overlap, 4);
    if (overlap[2] != 'a' || overlap[3] != 'b') return 7;

    // Test 7: Limit stops at exact string length
    for (i = 0; i < 16; i++) dst[i] = 0;
    strncpy(dst, "test", 4);
    if (dst[0] != 't' || dst[3] != 't') return 8;

    // Test 8: Buffer not null-terminated if limit reached without null
    for (i = 0; i < 16; i++) dst[i] = 0xFF;
    strncpy(dst, "hello", 5);
    // When limit is reached before null, no null is added
    if (dst[0] != 'h' || dst[4] != 'o') return 9;

    // Test 9: Copy empty string
    for (i = 0; i < 16; i++) dst[i] = 'X';
    strncpy(dst, "", 10);
    if (dst[0] != '\0') return 10;

    // Test 10: Different sizes
    for (i = 0; i < 16; i++) dst[i] = 0;
    strncpy(dst, "123456789", 3);
    if (dst[0] != '1' || dst[1] != '2' || dst[2] != '3') return 11;

    return 0;
}
