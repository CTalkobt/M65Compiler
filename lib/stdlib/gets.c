/* gets.c — gets / gets_s (<stdio.h>): a line from stdin (the keyboard,
 * through the KERNAL's screen editor: echoed and edited as BASIC's INPUT)
 * without its '\n'
 *
 * gets has no bound (C11 dropped it; cc65 and oscar64 keep it): prefer
 * gets_s or fgets. gets_s is C11's: n - 1 characters at most; a longer line
 * is read to its end, *s set to "" and NULL returned, as for s NULL or n 0.
 */

#include <stdio.h>

char *gets(char *s) {
    int c;
    char *p = s;
    while ((c = fgetc(stdin)) != EOF && c != '\n') *p++ = (char)c;
    if (c == EOF && p == s) return NULL;
    *p = 0;
    return s;
}

char *gets_s(char *s, size_t n) {
    int c;
    size_t i = 0;
    unsigned char over = 0;
    if (!s || !n) return NULL;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (i < n - 1) s[i++] = (char)c;
        else over = 1;
    }
    if (over || (c == EOF && !i)) {
        *s = 0;
        return NULL;
    }
    s[i] = 0;
    return s;
}
