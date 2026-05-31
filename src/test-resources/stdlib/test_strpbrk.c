// test_strpbrk: validate strpbrk() - find first matching character
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Match at start
    char *result = strpbrk("hello", "h");
    if (result == 0 || result[0] != 'h') return 1;

    // Test 2: Match in middle
    result = strpbrk("hello", "l");
    if (result == 0 || result[0] != 'l' || result[1] != 'l') return 2;
    if (result != &"hello"[2]) return 3;

    // Test 3: Match at end
    result = strpbrk("hello", "o");
    if (result == 0 || result[0] != 'o') return 4;

    // Test 4: No match
    result = strpbrk("hello", "xyz");
    if (result != 0) return 5;

    // Test 5: Multiple matching characters (returns first)
    result = strpbrk("hello", "ol");
    if (result == 0 || result[0] != 'l') return 6;

    // Test 6: Empty search set
    result = strpbrk("hello", "");
    if (result != 0) return 7;

    // Test 7: Empty string to search
    result = strpbrk("", "h");
    if (result != 0) return 8;

    // Test 8: Both empty
    result = strpbrk("", "");
    if (result != 0) return 9;

    // Test 9: Single character string
    result = strpbrk("a", "a");
    if (result == 0 || result[0] != 'a') return 10;

    // Test 10: Digit characters
    result = strpbrk("abc123def", "123");
    if (result == 0 || result[0] != '1') return 11;

    // Test 11: Special characters
    result = strpbrk("hello-world", "-");
    if (result == 0 || result[0] != '-') return 12;

    // Test 12: Case sensitivity
    result = strpbrk("Hello", "h");
    if (result != 0) return 13;

    // Test 13: Numbers and symbols
    result = strpbrk("test@123", "@");
    if (result == 0 || result[0] != '@') return 14;

    // Test 14: Space character
    result = strpbrk("hello world", " ");
    if (result == 0 || result[0] != ' ') return 15;

    // Test 15: Match multiple in set
    result = strpbrk("test", "st");
    if (result == 0 || result[0] != 's') return 16;

    // Test 16: Longer strings
    char long_str[32] = "The quick brown fox";
    result = strpbrk(long_str, "q");
    if (result == 0 || result[0] != 'q') return 17;

    // Test 17: Find space in longer string
    result = strpbrk(long_str, " ");
    if (result == 0 || result[0] != ' ') return 18;

    // Test 18: Numeric string search
    result = strpbrk("call 555-1234", "0123456789");
    if (result == 0 || result[0] != '5') return 19;

    // Test 19: Last character
    result = strpbrk("hello", "o");
    if (result == 0 || result[0] != 'o') return 20;

    // Test 20: Verify position
    char test[8] = "abcdef";
    result = strpbrk(test, "c");
    if (result == 0 || result != &test[2]) return 21;

    return 0;
}
