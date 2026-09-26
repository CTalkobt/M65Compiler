/* strtok_r.c — Reentrant string tokenizer for cc45 / MEGA65 */

#include <string.h>

/* strtok_r: reentrant token search
 * POSIX function, similar to strtok but thread-safe.
 * saveptr is passed by caller to maintain state between calls.
 * Returns: next token, or NULL when no more tokens
 */
char *strtok_r(char *s, const char *delim, char **saveptr) {
    char *end;

    if (s == NULL) {
        s = *saveptr;
    }

    if (s == NULL || *s == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    /* Skip leading delimiters */
    s += strspn(s, (char *)delim);
    if (*s == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    /* Find end of token */
    end = s + strcspn(s, (char *)delim);

    /* If we found a delimiter, terminate token and update saveptr */
    if (*end != '\0') {
        *end = '\0';
        *saveptr = end + 1;
    } else {
        *saveptr = NULL;
    }

    return s;
}
