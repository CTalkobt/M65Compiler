/* strqtok.c — strtok with quoted tokens (<string.h>, as cc65's)
 *
 * Like strtok, but a token may be enclosed in double quotes: it is then
 * the text between them, delimiters included, without the quotes. A quote
 * also ends an unquoted token (the quoted one is the next). A quoted token
 * without its closing quote runs to the end of the string.
 */

#include <string.h>

char *strqtok(char *s, const char *delim) {
    static char *next;
    static char quoted;          /* the next token starts after a quote */
    char *start;

    if (!s) {
        s = next;
        if (!s) return NULL;
        if (quoted) goto in_quote;
    }
    quoted = 0;
    while (*s && strchr(delim, *s)) s++;
    if (!*s) {
        next = s;
        return NULL;
    }
    if (*s == '"') {
        s++;
        goto in_quote;
    }
    start = s;
    while (*s && *s != '"' && !strchr(delim, *s)) s++;
    if (*s == '"') quoted = 1;
    if (*s) *s++ = 0;
    next = s;
    return start;

in_quote:
    quoted = 0;
    start = s;
    while (*s && *s != '"') s++;
    if (*s) *s++ = 0;
    next = s;
    return start;
}
