// test_strcspn: validate strcspn complement span
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Basic case - find position of first rejected char
    if (strcspn("hello world", " ") != 5) return 1;  // space at position 5

    // Test 2: No rejected chars found
    if (strcspn("hello", " ") != 5) return 2;  // return full length

    // Test 3: Rejected char at start
    if (strcspn(" hello", " ") != 0) return 3;  // space at position 0

    // Test 4: Multiple possible rejected chars
    if (strcspn("hello123", "123") != 5) return 4;  // '1' at position 5

    // Test 5: Empty reject set
    if (strcspn("hello", "") != 5) return 5;  // empty reject = full length

    // Test 6: All chars in reject set
    if (strcspn("abc", "abc") != 0) return 6;  // first char matches

    // Test 7: Multiple reject chars, first match
    if (strcspn("abcdef", "dx") != 3) return 7;  // 'd' at position 3

    // Test 8: Empty string
    if (strcspn("", "x") != 0) return 8;

    // Test 9: Complex reject set
    if (strcspn("the quick brown fox", "aeiou") != 2) return 9;  // 'e' at position 2

    // Test 10: Reject char at end
    if (strcspn("hello!", "!") != 5) return 10;

    return 0;  // Success
}
