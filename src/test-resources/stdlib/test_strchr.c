// test_strchr: validate strchr() and strrchr()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>
#include <stddef.h>

int main() {
    char *s = "hello world";
    char *result;

    // Test 1: strchr - find first occurrence
    result = strchr(s, 'o');
    if (result == NULL || *result != 'o') return 1;
    if (result != s + 4) return 2; // Should be at position 4 (l-l-o)

    // Test 2: strchr - character at start
    result = strchr(s, 'h');
    if (result == NULL || result != s) return 3;

    // Test 3: strchr - character not found
    result = strchr(s, 'z');
    if (result != NULL) return 4;

    // Test 4: strchr - null terminator
    result = strchr(s, '\0');
    if (result == NULL) return 5;
    if (*result != '\0') return 6;

    // Test 5: strrchr - find last occurrence
    result = strrchr(s, 'o');
    if (result == NULL || *result != 'o') return 7;
    if (result != s + 7) return 8; // Should be at position 7 (w-o-rld)

    // Test 6: strrchr - character at start (only occurrence)
    result = strrchr(s, 'h');
    if (result == NULL || result != s) return 9;

    // Test 7: strrchr - character not found
    result = strrchr(s, 'z');
    if (result != NULL) return 10;

    // Test 8: strrchr - null terminator
    result = strrchr(s, '\0');
    if (result == NULL) return 11;

    // Test 9: strchr on empty string
    result = strchr("", 'a');
    if (result != NULL) return 12;

    // Test 10: Find space character
    result = strchr(s, ' ');
    if (result == NULL || *result != ' ') return 13;
    if (result != s + 5) return 14;

    // Test 11: strrchr on single character
    result = strrchr("x", 'x');
    if (result == NULL) return 15;

    // Test 12: Repeated characters - strrchr gets last
    result = strrchr("aaaa", 'a');
    if (result == NULL) return 16;
    if (result != s + 3) return 17; // Last 'a' at position 3

    return 0;
}
