/* strndup.c — duplicate at most n characters of a string (<string.h>) */

#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n) {
    size_t len = 0;
    char *dup;
    while (len < n && s[len]) len++;
    dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, (void *)s, len);
        dup[len] = 0;
    }
    return dup;
}
