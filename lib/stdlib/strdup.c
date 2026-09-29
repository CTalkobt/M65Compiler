/* strdup.c — Duplicate a string */

#include <stdlib.h>
#include <string.h>

char *strdup(const char *s) {
    int len = strlen((char *)s);
    char *dup = (char *)malloc(len + 1);
    if (dup) {
        memcpy(dup, (void *)s, len + 1);
    }
    return dup;
}
