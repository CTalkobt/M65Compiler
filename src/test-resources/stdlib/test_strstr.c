// test_strstr: validate strstr()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>
#include <stddef.h>

int main() {
    char *haystack = "hello world hello";
    char *result;

    // Test 1: Find substring at start
    result = strstr(haystack, "hello");
    if (result == NULL) return 1;
    if (result != haystack) return 2;

    // Test 2: Find substring in middle
    result = strstr(haystack, "world");
    if (result == NULL) return 3;
    if (result != haystack + 6) return 4;

    // Test 3: Find substring at end
    result = strstr("foobar", "bar");
    if (result == NULL) return 5;
    if (strcmp(result, "bar") != 0) return 6;

    // Test 4: Substring not found
    result = strstr(haystack, "xyz");
    if (result != NULL) return 7;

    // Test 5: Empty substring (should find at start)
    result = strstr(haystack, "");
    if (result == NULL) return 8;
    if (result != haystack) return 9;

    // Test 6: Single character substring
    result = strstr("testing", "e");
    if (result == NULL) return 10;
    if (result != "testing" + 1) return 11;

    // Test 7: Substring same as haystack
    result = strstr("test", "test");
    if (result == NULL) return 12;

    // Test 8: Case sensitive
    result = strstr("Hello", "hello");
    if (result != NULL) return 13; // Should NOT find (case sensitive)

    // Test 9: Find first occurrence (not second)
    result = strstr(haystack, "hello");
    if (result != haystack) return 14; // Should find first at position 0, not at position 12

    // Test 10: Overlapping patterns
    result = strstr("aaaa", "aa");
    if (result == NULL) return 15;
    if (result != "aaaa") return 16; // Should find at start

    // Test 11: Two-character substring
    result = strstr("abcdef", "cd");
    if (result == NULL) return 17;
    if (strcmp(result, "cdef") != 0) return 18;

    // Test 12: Substring at end without null match
    result = strstr("prefix", "fix");
    if (result == NULL) return 19;
    if (strcmp(result, "fix") != 0) return 20;

    return 0;
}
