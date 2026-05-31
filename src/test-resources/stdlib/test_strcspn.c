// test_strcspn: validate strcspn() - count leading non-matching characters
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: No characters match
    if (strcspn("hello", "xyz") != 5) return 1;

    // Test 2: First character matches
    if (strcspn("hello", "h") != 0) return 2;

    // Test 3: Last character matches
    if (strcspn("hello", "o") != 4) return 3;

    // Test 4: Match in middle
    if (strcspn("hello", "l") != 2) return 4;

    // Test 5: Empty string
    if (strcspn("", "abc") != 0) return 5;

    // Test 6: Empty set
    if (strcspn("hello", "") != 5) return 6;

    // Test 7: Both empty
    if (strcspn("", "") != 0) return 7;

    // Test 8: Single character string
    if (strcspn("a", "a") != 0) return 8;

    // Test 9: Single character no match
    if (strcspn("a", "b") != 1) return 9;

    // Test 10: Numbers and letters
    if (strcspn("abc123", "0123456789") != 3) return 10;

    // Test 11: Case sensitivity
    if (strcspn("Hello", "h") != 5) return 11;

    // Test 12: Space character
    if (strcspn("hello world", " ") != 5) return 12;

    // Test 13: Special characters
    if (strcspn("test-value", "-") != 4) return 13;

    // Test 14: Multiple matching characters in set (returns first)
    if (strcspn("hello", "el") != 1) return 14;

    // Test 15: Stop at first match
    if (strcspn("aaabbbccc", "b") != 3) return 15;

    // Test 16: All characters in set
    if (strcspn("abc", "abc") != 0) return 16;

    // Test 17: Match at different position
    if (strcspn("12345", "45") != 3) return 17;

    // Test 18: Longer string
    char long_str[32] = "abcdefghij:klmnop";
    if (strcspn(long_str, ":") != 10) return 18;

    // Test 19: Digit at end
    if (strcspn("test123", "123") != 4) return 19;

    // Test 20: Repeated character in set
    if (strcspn("aaaaab", "b") != 5) return 20;

    return 0;
}
