/* strspn.c — Length of initial segment matching accept characters */

int strspn(char *restrict s, char *restrict accept) {
    int count = 0;
    char *a;
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
