// test_strpbrk: validate strpbrk character search
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    const char *str = "hello world";
    const char *result;

    // Test 1: Find first matching char
    result = strpbrk(str, "aeiou");
    if (result == NULL || result[0] != 'e') return 1;

    // Test 2: Single char to find
    result = strpbrk(str, "w");
    if (result == NULL || result[0] != 'w') return 2;

    // Test 3: No matching char
    result = strpbrk(str, "xyz");
    if (result != NULL) return 3;

    // Test 4: Match at start
    result = strpbrk(str, "h");
    if (result == NULL || result[0] != 'h') return 4;

    // Test 5: Match at end
    result = strpbrk(str, "d");
    if (result == NULL || result[0] != 'd') return 5;

    // Test 6: Multiple chars in accept, first match wins
    result = strpbrk("abcdef", "zxew");
    if (result == NULL || result[0] != 'e') return 6;

    // Test 7: Empty accept set
    result = strpbrk(str, "");
    if (result != NULL) return 7;

    // Test 8: Empty string
    result = strpbrk("", "abc");
    if (result != NULL) return 8;

    // Test 9: Multiple possible matches, first in string
    result = strpbrk("hello", "ol");
    if (result == NULL || result[0] != 'l') return 9;  // 'l' at position 2

    // Test 10: Whitespace matching
    result = strpbrk("hello world", " ");
    if (result == NULL || result[0] != ' ') return 10;

    return 0;  // Success
}
