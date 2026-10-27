/* strpbrk.c — Find first occurrence of any character from accept in s */

char *strpbrk(const char *s, const char *accept) {
    const char *a;
    while (*s) {
        a = accept;
        while (*a) {
            if (*s == *a) return (char *)s;
            a++;
        }
        s++;
    }
    return (char *)0;
}
