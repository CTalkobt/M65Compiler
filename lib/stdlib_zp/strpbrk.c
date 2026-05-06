/* strpbrk.c — Find first occurrence of any character from accept in s */

char *strpbrk(char *s, char *accept) {
    char *a;
    while (*s) {
        a = accept;
        while (*a) {
            if (*s == *a) return s;
            a++;
        }
        s++;
    }
    return (char *)0;
}
