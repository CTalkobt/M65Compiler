/* memchr.c — Find byte in memory for cc45 / MEGA65 */

void *memchr(void *s, int c, int n) {
    char *p = (char *)s;
    char uc = (char)c;
    int i;
    for (i = 0; i < n; i++) {
        if (p[i] == uc)
            return (void *)(p + i);
    }
    return (void *)0;
}
