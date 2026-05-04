// Example: strchr/strrchr — validate character search
// Uses c45.lib string.h implementation.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

char msg[8];

int main() {
    msg[0] = 'a';
    msg[1] = 'b';
    msg[2] = 'c';
    msg[3] = 'b';
    msg[4] = 'd';
    msg[5] = 0;

    // strchr: find first 'b'
    char *p = strchr(msg, 'b');
    if (p == (char *)0) return 1;
    if (p != msg + 1) return 2;

    // strchr: not found
    if (strchr(msg, 'z') != (char *)0) return 3;

    // strrchr: find last 'b'
    p = strrchr(msg, 'b');
    if (p == (char *)0) return 4;
    if (p != msg + 3) return 5;

    return 0;
}
