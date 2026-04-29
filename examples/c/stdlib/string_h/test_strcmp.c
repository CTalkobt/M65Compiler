// Example: strcmp — validate string comparison
// Uses stdlib45.lib string.h implementation.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Equal strings
    if (strcmp("abc", "abc") != 0) return 1;
    if (strcmp("", "") != 0) return 2;

    // Less-than / greater-than
    if (strcmp("abc", "abd") >= 0) return 3;
    if (strcmp("abd", "abc") <= 0) return 4;

    // Prefix comparisons
    if (strcmp("ab", "abc") >= 0) return 5;
    if (strcmp("abc", "ab") <= 0) return 6;

    return 0;
}
