/* strtof.c — String to float conversion for cc45 / MEGA65
 *
 * float strtof(char *nptr, char **endptr);
 * float atof(char *nptr);
 *
 * Parses: [whitespace] [sign] [digits] [.digits] [e/E [sign] digits]
 * Sets *endptr to first unconsumed character (if endptr != NULL).
 */

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
    int gotDigit = 0;
    while (*s >= '0' && *s <= '9') {
        result = result * 10.0 + (float)(*s - '0');
        s = s + 1;
        gotDigit = 1;
    }

    /* Fractional part */
    if (*s == '.') {
        s = s + 1;
        float frac = 0.1;
        while (*s >= '0' && *s <= '9') {
            result = result + (float)(*s - '0') * frac;
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
            exp = exp * 10 + (*s - '0');
            s = s + 1;
        }
        /* Apply exponent: multiply or divide by 10^exp */
        float power = 1.0;
        while (exp > 0) {
            power = power * 10.0;
            exp = exp - 1;
        }
        if (expNeg) result = result / power;
        else result = result * power;
    }

    if (neg) result = 0.0 - result;

    if (endptr) *endptr = s;
    return result;
}

float atof(char *nptr) {
    return strtof(nptr, 0);
}
