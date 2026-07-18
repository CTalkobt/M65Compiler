// test_strlcpy: validate safe string copy
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char buffer[16];
    const char *src = "hello";
    const char *long_src = "this is a very long string";

    // Test 1: Normal copy within buffer
    int ret = strlcpy(buffer, src, 16);
    if (ret != 5) return 1;  // strlcpy returns length of src
    if (strcmp(buffer, "hello") != 0) return 2;

    // Test 2: Truncation when src too long
    ret = strlcpy(buffer, long_src, 6);
    if (ret != 26) return 3;  // Should return full src length
    if (strcmp(buffer, "this ") != 0) return 4;  // But only 5 chars copied (size-1)
    if (buffer[5] != '\0') return 5;  // Must be null-terminated

    // Test 3: Exact fit
    ret = strlcpy(buffer, "hello", 6);
    if (ret != 5) return 6;
    if (strcmp(buffer, "hello") != 0) return 7;

    // Test 4: Zero-length buffer (size=0)
    ret = strlcpy(buffer, src, 0);
    if (ret != 5) return 8;  // Should still return src length
    // Buffer should be unchanged (nothing written to it)

    // Test 5: Single byte buffer (only room for null terminator)
    ret = strlcpy(buffer, "x", 1);
    if (ret != 1) return 9;
    if (buffer[0] != '\0') return 10;  // Only null terminator written

    // Test 6: Empty source
    ret = strlcpy(buffer, "", 16);
    if (ret != 0) return 11;
    if (buffer[0] != '\0') return 12;

    return 0;  // Success
}
