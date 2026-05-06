/* strstr.c — Locate substring */

char *strstr(char *haystack, char *needle) {
    char *h;
    char *n;
    char *start;

    if (*needle == 0) return haystack;

    while (*haystack) {
        start = haystack;
        h = haystack;
        n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (*n == 0) return start;
        haystack++;
    }
    return (char *)0;
}
