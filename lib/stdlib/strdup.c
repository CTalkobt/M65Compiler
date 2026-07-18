/* strdup.c — Allocate and duplicate string for cc45 / MEGA65 */

#include <stdlib.h>
#include <string.h>

/* strdup: allocate and copy string
 * POSIX function, returns malloc'd copy of s.
 * Returns: pointer to allocated string, or NULL on malloc failure
 */
char *strdup(const char *s) {
    int len = strlen((char *)s) + 1;
    char *copy = (char *)malloc(len);
    if (copy) {
        strcpy(copy, (char *)s);
    }
    return copy;
}
