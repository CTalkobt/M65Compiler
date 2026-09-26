// test_strstr: validate strstr substring search
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    const char *haystack = "hello world";
    const char *result;

    // Test 1: Find substring at start
    result = strstr(haystack, "hello");
    if (result != haystack) return 1;

    // Test 2: Find substring in middle
    result = strstr(haystack, "lo w");
    if (result == NULL || result[0] != 'l') return 2;

    // Test 3: Find single char
    result = strstr(haystack, "w");
    if (result == NULL || result[0] != 'w') return 3;

    // Test 4: Substring not found
    result = strstr(haystack, "xyz");
    if (result != NULL) return 4;

    // Test 5: Empty needle returns haystack
    result = strstr(haystack, "");
    if (result != haystack) return 5;

    // Test 6: Find at end
    result = strstr(haystack, "d");
    if (result == NULL || result[0] != 'd') return 6;

    // Test 7: Multi-char substring
    result = strstr(haystack, "world");
    if (result == NULL || result[0] != 'w') return 7;

    // Test 8: Substring same as haystack
    result = strstr("abc", "abc");
    if (result == NULL) return 8;

    // Test 9: Partial match then full match
    result = strstr("aaab", "aab");
    if (result == NULL) return 9;

    // Test 10: Empty haystack
    result = strstr("", "x");
    if (result != NULL) return 10;

    return 0;  // Success
}
