// test_strnlen: validate bounded string length
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    const char *str = "hello";
    const char *long_str = "this is a longer string";
    const char *empty = "";

    // Test 1: String shorter than maxlen
    if (strnlen(str, 10) != 5) return 1;

    // Test 2: String at exactly maxlen
    if (strnlen(str, 5) != 5) return 2;

    // Test 3: String longer than maxlen (should return maxlen)
    if (strnlen(str, 3) != 3) return 3;

    // Test 4: Longer string, partial length
    if (strnlen(long_str, 4) != 4) return 4;

    // Test 5: Longer string, full length
    if (strnlen(long_str, 23) != 23) return 5;

    // Test 6: Empty string
    if (strnlen(empty, 10) != 0) return 6;

    // Test 7: Empty string with maxlen=0
    if (strnlen(empty, 0) != 0) return 7;

    // Test 8: Non-empty string with maxlen=0
    if (strnlen(str, 0) != 0) return 8;

    return 0;  // Success
}
