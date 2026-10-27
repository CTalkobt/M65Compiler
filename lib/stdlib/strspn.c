/* strspn.c — Length of initial segment matching accept characters */

int strspn(const char *s, const char *accept) {
    int count = 0;
    const char *a;
    int found;
    while (*s) {
        found = 0;
        a = accept;
        while (*a) {
            if (*s == *a) { found = 1; break; }
            a++;
        }
        if (!found) return count;
        count++;
        s++;
    }
    return count;
}
