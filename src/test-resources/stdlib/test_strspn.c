// test_strspn: validate strspn() - count leading matching characters
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: All characters match
    if (strspn("hello", "helo") != 5) return 1;

    // Test 2: No characters match
    if (strspn("hello", "xyz") != 0) return 2;

    // Test 3: Partial match
    if (strspn("hello", "hel") != 3) return 3;

    // Test 4: Single matching character
    if (strspn("hello", "h") != 1) return 4;

    // Test 5: Empty string
    if (strspn("", "abc") != 0) return 5;

    // Test 6: Empty set
    if (strspn("hello", "") != 0) return 6;

    // Test 7: Both empty
    if (strspn("", "") != 0) return 7;

    // Test 8: Digits
    if (strspn("12345", "0123456789") != 5) return 8;

    // Test 9: Mixed digits and letters
    if (strspn("123abc", "123") != 3) return 9;

    // Test 10: Single character string and set
    if (strspn("a", "a") != 1) return 10;

    // Test 11: Longer string, short match
    if (strspn("abcdefgh", "ab") != 2) return 11;

    // Test 12: Case sensitivity
    if (strspn("Hello", "hello") != 0) return 12;

    // Test 13: Space character
    if (strspn("   hello", " ") != 3) return 13;

    // Test 14: Numbers and symbols
    if (strspn("***test", "*") != 3) return 14;

    // Test 15: Alternating pattern
    if (strspn("ababab", "ab") != 6) return 15;

    // Test 16: Different order in set
    if (strspn("hello", "ehlo") != 5) return 16;

    // Test 17: Repeated characters in set
    if (strspn("aaa", "aaa") != 3) return 17;

    // Test 18: Long string
    char long_str[32] = "1234567890abcdef";
    if (strspn(long_str, "0123456789") != 10) return 18;

    // Test 19: Stop at first mismatch
    if (strspn("aaabbbccc", "ab") != 6) return 19;

    // Test 20: Single character with set containing multiple
    if (strspn("x", "xyz") != 1) return 20;

    return 0;
}
