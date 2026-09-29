/* strnlen.c — Length-limited string length */

int strnlen(const char *s, int maxlen) {
    int len = 0;
    while (len < maxlen && s[len]) {
        len++;
    }
    return len;
}
