/* strtoul.c — Convert string to unsigned long
 *
 * Letters are digits 10-35 in either case and in any of the encodings a
 * string can have: cc45 literals ("ff" $46, "FF" $66), ASCII and shifted
 * PETSCII ($C1-$DA). A value that does not fit gives ULONG_MAX and errno
 * ERANGE; a leading '-' negates the result (as C specifies).
 */
#include <errno.h>
#include <limits.h>

/* Digit value of c, or 99 */
static int __strto_digit(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 0x41 && c <= 0x5A) return c - 0x41 + 10;
    if (c >= 0x61 && c <= 0x7A) return c - 0x61 + 10;
    if (c >= 0xC1 && c <= 0xDA) return c - 0xC1 + 10;
    return 99;
}

static int __strto_isx(unsigned char c) {
    return c == 0x58 || c == 0x78 || c == 0xD8;
}

unsigned long strtoul(char *nptr, char **endptr, int base) {
    unsigned long result = 0L;
    unsigned char *s = (unsigned char *)nptr;
    int digit;
    int any = 0, overflow = 0, neg = 0;
    unsigned long cutoff;
    int cutlim;

    /* Skip leading whitespace */
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s++;

    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;

    if (base < 0 || base == 1 || base > 36) {
        if (endptr) *endptr = nptr;
        errno = EINVAL;
        return 0;
    }
    /* 0x prefix (only when a hex digit follows it) */
    if ((base == 0 || base == 16) && s[0] == '0' && __strto_isx(s[1]) && __strto_digit(s[2]) < 16) {
        base = 16;
        s += 2;
    } else if (base == 0) {
        base = *s == '0' ? 8 : 10;
    }

    cutoff = ULONG_MAX / (unsigned long)base;
    cutlim = (int)(ULONG_MAX % (unsigned long)base);
    while ((digit = __strto_digit(*s)) < base) {
        if (result > cutoff || (result == cutoff && digit > cutlim))
            overflow = 1;
        else
            result = result * (unsigned long)base + (unsigned long)digit;
        any = 1;
        s++;
    }

    if (endptr) *endptr = any ? (char *)s : nptr;
    if (overflow) {
        errno = ERANGE;
        return ULONG_MAX;
    }
    return neg ? -result : result;
}
