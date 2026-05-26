// test_strcmp: validate strcmp()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    if (strcmp("abc", "abc") != 0) return 1;
    if (strcmp("abc", "abd") >= 0) return 2;
    if (strcmp("abd", "abc") <= 0) return 3;
    if (strcmp("", "") != 0) return 4;
    if (strcmp("a", "") <= 0) return 5;
    if (strcmp("", "a") >= 0) return 6;
    if (strcmp("ab", "abc") >= 0) return 7;
    if (strcmp("abc", "ab") <= 0) return 8;
    return 0;
}
