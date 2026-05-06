/* strtoul.c — Convert string to unsigned long */

unsigned long strtoul(char *nptr, char **endptr, int base) {
    unsigned long result = 0L;
    char *s = nptr;
    int digit;
    int any = 0;

    /* Skip leading whitespace */
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;

    /* Skip optional '+' */
    if (*s == '+') s++;

    /* Auto-detect base */
    if (base == 0) {
        if (*s == '0') {
            s++;
            if (*s == 'x' || *s == 'X' ||
                *s == 0x58 || *s == 0xD8) {  /* PETSCII X */
                base = 16;
                s++;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (*s == '0' && (s[1] == 'x' || s[1] == 'X' ||
                          s[1] == 0x58 || s[1] == 0xD8)) {
            s += 2;
        }
    }

    /* Parse digits */
    while (*s) {
        if (*s >= '0' && *s <= '9') {
            digit = *s - '0';
        } else if (*s >= 0x41 && *s <= 0x5A) {
            /* PETSCII lowercase a-z */
            digit = *s - 0x41 + 10;
        } else if (*s >= 0xC1 && *s <= 0xDA) {
            /* PETSCII uppercase A-Z */
            digit = *s - 0xC1 + 10;
        } else {
            break;
        }
        if (digit >= base) break;
        result = result * (unsigned long)base + (unsigned long)digit;
        any = 1;
        s++;
    }

    if (endptr != (char **)0) {
        *endptr = any ? s : nptr;
    }

    return result;
}
