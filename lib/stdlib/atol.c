/* atol.c — Convert string to long integer */

long strtol(char *nptr, char **endptr, int base);

long atol(char *s) {
    return strtol(s, (char **)0, 10);
}
