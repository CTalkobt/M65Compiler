/* strnlen.c — Bounded string length for cc45 / MEGA65 */

#include <string.h>

/* strnlen: return length of s, up to maxlen
 * POSIX extension, often used for safe string handling.
 */
int strnlen(const char *s, int maxlen) {
    int len = 0;
    while (len < maxlen && s[len] != '\0') {
        len++;
    }
    return len;
}
