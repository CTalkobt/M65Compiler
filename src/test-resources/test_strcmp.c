// Test: strcmp — validate string comparison
// Returns 0 (A=$00) on success, non-zero on failure.

int strcmp(char *s1, char *s2) {
    while (*s1 != 0) {
        if (*s1 != *s2) {
            if (*s1 < *s2) return -1;
            return 1;
        }
        s1 = s1 + 1;
        s2 = s2 + 1;
    }
    if (*s2 != 0) return -1;
    return 0;
}

int main() {
    if (strcmp("abc", "abc") != 0) return 1;
    if (strcmp("", "") != 0) return 2;
    if (strcmp("abc", "abd") >= 0) return 3;
    if (strcmp("abd", "abc") <= 0) return 4;
    if (strcmp("ab", "abc") >= 0) return 5;
    if (strcmp("abc", "ab") <= 0) return 6;
    return 0;
}
