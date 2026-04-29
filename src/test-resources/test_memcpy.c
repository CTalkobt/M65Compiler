// Test: memcpy/memset/memcmp — validate memory operations
// Returns 0 (A=$00) on success, non-zero on failure.

char *memset(char *s, int c, int n) {
    char *p = s;
    while (n > 0) {
        *p = (char)c;
        p = p + 1;
        n = n - 1;
    }
    return s;
}

char *memcpy(char *dest, char *src, int n) {
    char *d = dest;
    char *s = src;
    while (n > 0) {
        *d = *s;
        d = d + 1;
        s = s + 1;
        n = n - 1;
    }
    return dest;
}

int memcmp(char *s1, char *s2, int n) {
    while (n > 0) {
        if (*s1 != *s2) {
            if (*s1 < *s2) return -1;
            return 1;
        }
        s1 = s1 + 1;
        s2 = s2 + 1;
        n = n - 1;
    }
    return 0;
}

char src[8];
char dst[8];

int main() {
    // memset test
    memset(src, 0x42, 4);
    if (src[0] != 0x42) return 1;
    if (src[3] != 0x42) return 2;

    // memcpy test
    memset(dst, 0, 8);
    memcpy(dst, src, 4);
    if (dst[0] != 0x42) return 3;
    if (dst[3] != 0x42) return 4;

    // memcmp test
    if (memcmp(src, dst, 4) != 0) return 5;
    dst[2] = 0x41;
    if (memcmp(src, dst, 4) <= 0) return 6;

    return 0;
}
