// Test: strchr/strrchr — validate character search
// Returns 0 (A=$00) on success, non-zero on failure.

char *strchr(char *s, int c) {
    while (*s != 0) {
        if (*s == (char)c) return s;
        s = s + 1;
    }
    if ((char)c == 0) return s;
    return (char *)0;
}

char *strrchr(char *s, int c) {
    char *last = (char *)0;
    while (*s != 0) {
        if (*s == (char)c) last = s;
        s = s + 1;
    }
    if ((char)c == 0) return s;
    return last;
}

char msg[8];

int main() {
    msg[0] = 'a';
    msg[1] = 'b';
    msg[2] = 'c';
    msg[3] = 'b';
    msg[4] = 'd';
    msg[5] = 0;

    // strchr: find first 'b'
    char *p = strchr(msg, 'b');
    if (p == (char *)0) return 1;
    if (p != msg + 1) return 2;

    // strchr: not found
    if (strchr(msg, 'z') != (char *)0) return 3;

    // strrchr: find last 'b'
    p = strrchr(msg, 'b');
    if (p == (char *)0) return 4;
    if (p != msg + 3) return 5;

    return 0;
}
