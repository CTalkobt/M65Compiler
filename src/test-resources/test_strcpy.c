// Test: strcpy — validate string copy
// Returns 0 (A=$00) on success, non-zero on failure.

char *strcpy(char *dest, char *src) {
    char *ret = dest;
    while (*src != 0) {
        *dest = *src;
        dest = dest + 1;
        src = src + 1;
    }
    *dest = 0;
    return ret;
}

int strlen(char *s) {
    int len = 0;
    while (*s != 0) { len = len + 1; s = s + 1; }
    return len;
}

int strcmp(char *s1, char *s2) {
    while (*s1 != 0) {
        if (*s1 != *s2) return 1;
        s1 = s1 + 1;
        s2 = s2 + 1;
    }
    if (*s2 != 0) return 1;
    return 0;
}

char buf[16];

int main() {
    strcpy(buf, "hello");
    if (strcmp(buf, "hello") != 0) return 1;
    if (strlen(buf) != 5) return 2;
    strcpy(buf, "");
    if (strlen(buf) != 0) return 3;
    return 0;
}
