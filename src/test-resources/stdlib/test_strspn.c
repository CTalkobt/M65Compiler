// test_strspn: validate strspn span
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Basic case - count chars at start that match accept set
    if (strspn("hello world", "helo") != 4) return 1;  // 'h','e','l','l' match; space breaks

    // Test 2: No chars in accept set
    if (strspn("hello", "xyz") != 0) return 2;  // no match

    // Test 3: All chars in accept set
    if (strspn("hello", "ehlo") != 5) return 3;  // all match

    // Test 4: Accept char at start only
    if (strspn("abcdef", "ax") != 1) return 4;  // 'a' matches, 'b' doesn't

    // Test 5: Empty accept set
    if (strspn("hello", "") != 0) return 5;  // empty accept = no match

    // Test 6: Complex accept set with digits
    if (strspn("12345abc", "12345") != 5) return 6;  // all digits match

    // Test 7: Accept set with space
    if (strspn("   hello", " ") != 3) return 7;  // 3 spaces

    // Test 8: Empty string
    if (strspn("", "abc") != 0) return 8;

    // Test 9: Single char that matches
    if (strspn("a", "a") != 1) return 9;

    // Test 10: Single char that doesn't match
    if (strspn("a", "b") != 0) return 10;

    // Test 11: Multiple char accept set, multiple matches
    if (strspn("aabbcc", "abc") != 6) return 11;  // all 6 chars match

    // Test 12: Accept set matches, then breaks
    if (strspn("aaabbb", "ab") != 6) return 12;  // all 6 match 'ab' set

    return 0;  // Success
}
