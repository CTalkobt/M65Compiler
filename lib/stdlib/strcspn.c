/* strcspn.c — Length of initial segment not matching reject characters */

int strcspn(char *s, char *reject) {
    int count = 0;
    char *r;
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
