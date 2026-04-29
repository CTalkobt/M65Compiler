// Test: strlen — validate string length calculation
// Returns 0 (A=$00) on success, non-zero on failure.

int strlen(char *s) {
    int len = 0;
    while (*s != 0) {
        len = len + 1;
        s = s + 1;
    }
    return len;
}

int main() {
    if (strlen("hello") != 5) return 1;
    if (strlen("") != 0) return 2;
    if (strlen("a") != 1) return 3;
    if (strlen("abcdefghij") != 10) return 4;
    return 0;
}
