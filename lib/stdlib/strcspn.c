/* strcspn.c — Length of initial segment not matching reject characters */

int strcspn(const char *s, const char *reject) {
    int count = 0;
    const char *r;
    while (*s) {
        r = reject;
        while (*r) {
            if (*s == *r) return count;
            r++;
        }
        count++;
        s++;
    }
    return count;
}
