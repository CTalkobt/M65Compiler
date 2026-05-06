/* strtol.c — Convert string to long */

unsigned long strtoul(char *nptr, char **endptr, int base);

long strtol(char *nptr, char **endptr, int base) {
    char *s = nptr;
    int neg = 0;
    long result;

    /* Skip leading whitespace */
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;

    /* Check sign */
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    result = (long)strtoul(s, endptr, base);

    if (neg) result = -result;

    /* If no digits were consumed, endptr should point to nptr */
    if (endptr != (char **)0 && *endptr == s) {
        *endptr = nptr;
    }

    return result;
}
