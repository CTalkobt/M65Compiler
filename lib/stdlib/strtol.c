/* strtol.c — Convert string to long
 *
 * A value outside LONG_MIN..LONG_MAX gives LONG_MIN or LONG_MAX and errno
 * ERANGE.
 */
#include <errno.h>
#include <limits.h>

unsigned long strtoul(char *nptr, char **endptr, int base);

long strtol(char *nptr, char **endptr, int base) {
    char *s = nptr;
    int neg = 0;
    unsigned long u;
    int saved = errno;

    /* Skip leading whitespace */
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;

    /* Check sign */
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    /* A second sign is not a number */
    if (*s == '-' || *s == '+') {
        if (endptr) *endptr = nptr;
        return 0;
    }

    errno = 0;
    u = strtoul(s, endptr, base);
    /* No digits: endptr is the original string */
    if (endptr && *endptr == s) *endptr = nptr;
    if (errno == ERANGE || (!neg && u > (unsigned long)LONG_MAX) || (neg && u > (unsigned long)LONG_MAX + 1UL)) {
        errno = ERANGE;
        return neg ? LONG_MIN : LONG_MAX;
    }
    if (errno == 0) errno = saved;
    return neg ? -(long)u : (long)u;
}
