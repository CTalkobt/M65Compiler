/* strtok.c — Tokenize string */

static char *_strtok_next;

char *strtok(char *s, char *delim) {
    char *start;
    char *d;
    int is_delim;

    if (s != (char *)0) {
        _strtok_next = s;
    }

    if (_strtok_next == (char *)0) return (char *)0;

    /* Skip leading delimiters */
    while (*_strtok_next) {
        is_delim = 0;
        d = delim;
        while (*d) {
            if (*_strtok_next == *d) { is_delim = 1; break; }
            d++;
        }
        if (!is_delim) break;
        _strtok_next++;
    }

    if (*_strtok_next == 0) {
        _strtok_next = (char *)0;
        return (char *)0;
    }

    start = _strtok_next;

    /* Find end of token */
    while (*_strtok_next) {
        d = delim;
        while (*d) {
            if (*_strtok_next == *d) {
                *_strtok_next = 0;
                _strtok_next++;
                return start;
            }
            d++;
        }
        _strtok_next++;
    }

    _strtok_next = (char *)0;
    return start;
}
