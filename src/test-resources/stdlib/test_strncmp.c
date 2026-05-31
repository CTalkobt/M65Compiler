// test_strncmp: validate strncmp() - limited-length string comparison
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Equal strings with exact length
    if (strncmp("hello", "hello", 5) != 0) return 1;

    // Test 2: Equal within limit
    if (strncmp("hello", "hello world", 5) != 0) return 2;

    // Test 3: First is less than second
    int cmp = strncmp("abc", "abd", 3);
    if (cmp >= 0) return 3;

    // Test 4: First is greater than second
    cmp = strncmp("abd", "abc", 3);
    if (cmp <= 0) return 4;

    // Test 5: Different at first character
    cmp = strncmp("xyz", "abc", 1);
    if (cmp <= 0) return 5; // 'x' > 'a'

    // Test 6: Zero length comparison (should be equal)
    if (strncmp("anything", "different", 0) != 0) return 6;

    // Test 7: Single character comparison
    if (strncmp("a", "a", 1) != 0) return 7;

    // Test 8: Comparison stops at limit
    if (strncmp("hello", "help", 3) != 0) return 8; // "hel" == "hel"

    // Test 9: Partial mismatch
    cmp = strncmp("test", "team", 2);
    if (cmp != 0) return 9; // "te" == "te"

    // Test 10: Case sensitive
    cmp = strncmp("Hello", "hello", 5);
    if (cmp >= 0) return 10; // 'H' < 'h'

    // Test 11: Empty strings
    if (strncmp("", "", 0) != 0) return 11;
    if (strncmp("", "", 5) != 0) return 12;

    // Test 12: One empty string
    cmp = strncmp("test", "", 1);
    if (cmp <= 0) return 13;

    // Test 13: Limit larger than string length
    if (strncmp("hi", "hi", 100) != 0) return 14;

    // Test 14: Numbers in strings
    if (strncmp("123", "123", 3) != 0) return 15;
    cmp = strncmp("100", "200", 1);
    if (cmp >= 0) return 16; // '1' < '2'

    // Test 15: Special characters
    if (strncmp("a.b", "a.b", 3) != 0) return 17;
    cmp = strncmp("a-b", "a+b", 3);
    if (cmp >= 0) return 18; // '-' < '+'

    return 0;
}
