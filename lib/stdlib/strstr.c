/* strstr.c — Locate substring */

char *strstr(const char *haystack, const char *needle) {
    const char *h;
    const char *n;
    const char *start;

    if (*needle == 0) return (char *)haystack;

    while (*haystack) {
        start = haystack;
        h = haystack;
        n = needle;
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        if (*n == 0) return (char *)start;
        haystack++;
    }
    return (char *)0;
}
