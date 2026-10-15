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

/* Exact powers of ten (the 32-bit mantissa holds 10^9 exactly) */
static const float sf_pow10[10] = { 1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0,
                                    1000000.0, 10000000.0, 100000000.0, 1000000000.0 };

float strtof(char *nptr, char **endptr) {
    char *s = nptr;

    /* Skip leading whitespace */
    while (*s == ' ' || (*s >= 9 && *s <= 13)) s = s + 1;

    /* Sign */
    int neg = 0;
    if (*s == '-') { neg = 1; s = s + 1; }
    else if (*s == '+') { s = s + 1; }

    /* Digits form an exact integer mantissa (up to 10 digits; later integer
       digits only raise the exponent, later fraction digits are dropped),
       scaled by a power of ten at the end: 0.25 is 25 / 100, exactly */
    float result = 0.0;
    int gotDigit = 0, nonZero = 0;
    int e10 = 0;
    while (*s >= '0' && *s <= '9') {
        if (result < 1000000000.0) result = result * 10.0 + (float)(*s - '0');
        else e10 = e10 + 1;
        if (*s != '0') nonZero = 1;
        s = s + 1;
        gotDigit = 1;
    }
    if (*s == '.') {
        char *dot = s;
        s = s + 1;
        while (*s >= '0' && *s <= '9') {
            if (result < 1000000000.0) { result = result * 10.0 + (float)(*s - '0'); e10 = e10 - 1; }
            if (*s != '0') nonZero = 1;
            s = s + 1;
            gotDigit = 1;
        }
        if (!gotDigit) s = dot;
    }
    if (!gotDigit) {
        if (endptr) *endptr = nptr;
        return 0.0;
    }

    /* Exponent part (only when digits follow the e) */
    if (*s == 'e' || *s == 'E') {
        char *e = s + 1;
        int expNeg = 0;
        if (*e == '-') { expNeg = 1; e = e + 1; }
        else if (*e == '+') { e = e + 1; }
        if (*e >= '0' && *e <= '9') {
            int exp = 0;
            while (*e >= '0' && *e <= '9') {
                /* past 200 the result is out of range either way */
                if (exp < 200) exp = exp * 10 + (*e - '0');
                e = e + 1;
            }
            e10 = expNeg ? e10 - exp : e10 + exp;
            s = e;
        }
    }

    /* Scale in steps of up to 10^9, so underflow reaches 0 and overflow
       stops at the largest float */
    while (e10 > 0 && result != 0.0 && !sf_is_max(result)) {
        int k = e10 > 9 ? 9 : e10;
        result = result * sf_pow10[k];
        e10 = e10 - k;
    }
    while (e10 < 0 && result != 0.0) {
        int k = e10 < -9 ? 9 : -e10;
        result = result / sf_pow10[k];
        e10 = e10 + k;
    }

    if (sf_is_max(result) || (result == 0.0 && nonZero)) errno = ERANGE;

    if (neg) result = 0.0 - result;

    if (endptr) *endptr = s;
    return result;
}

float atof(char *nptr) {
    return strtof(nptr, 0);
}
