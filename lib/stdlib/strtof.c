/* strtof.c — String to float conversion for cc45 / MEGA65
 *
 * float strtof(char *nptr, char **endptr);
 * float atof(char *nptr);
 *
 * Parses: [whitespace] [sign] [digits] [.digits] [e/E [sign] digits]
 * Sets *endptr to first unconsumed character (if endptr != NULL).
 * A value beyond the CBM float range gives the largest float (overflow) or 0
 * (underflow), with errno = ERANGE.
 */

#include <errno.h>

/* The saturated result of an overflowing operation: the largest float */
static int sf_is_max(float f) {
    unsigned char *p = (unsigned char *)&f;
    return p[0] == 0xFF && (p[1] & 0x7F) == 0x7F && p[2] == 0xFF && p[3] == 0xFF && p[4] == 0xFF;
}

float strtof(char *nptr, char **endptr) {
    char *s = nptr;

    /* Skip leading whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s = s + 1;

    /* Sign */
    int neg = 0;
    if (*s == '-') { neg = 1; s = s + 1; }
    else if (*s == '+') { s = s + 1; }

    /* Integer part */
    float result = 0.0;
    int gotDigit = 0, nonZero = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10.0 + (float)(*s - '0');
        if (*s != '0') nonZero = 1;
        s = s + 1;
        gotDigit = 1;
    }

    /* Fractional part */
    if (*s == '.') {
        s = s + 1;
        float frac = 0.1;
        while (*s >= '0' && *s <= '9') {
            result = result + (float)(*s - '0') * frac;
            if (*s != '0') nonZero = 1;
            frac = frac * 0.1;
            s = s + 1;
            gotDigit = 1;
        }
    }

    /* Exponent part */
    if (gotDigit && (*s == 'e' || *s == 'E')) {
        s = s + 1;
        int expNeg = 0;
        if (*s == '-') { expNeg = 1; s = s + 1; }
        else if (*s == '+') { s = s + 1; }
        int exp = 0;
        while (*s >= '0' && *s <= '9') {
            /* past 120 the result is out of range either way */
            if (exp < 120) exp = exp * 10 + (*s - '0');
            s = s + 1;
        }
        if (exp > 120) exp = 120;
        /* One step at a time, so underflow reaches 0 and overflow stops at
           the largest float instead of dividing by a saturated power */
        while (exp > 0 && result != 0.0 && !sf_is_max(result)) {
            if (expNeg) result = result / 10.0;
            else result = result * 10.0;
            exp = exp - 1;
        }
    }

    if (sf_is_max(result) || (result == 0.0 && nonZero)) errno = ERANGE;

    if (neg) result = 0.0 - result;

    if (endptr) *endptr = s;
    return result;
}

float atof(char *nptr) {
    return strtof(nptr, 0);
}
