/* perror.c — Print error message to stderr */

#include <stdio.h>
#include <errno.h>
#include <string.h>

void perror(const char *s) {
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs(strerror(errno), stderr);
    fputc('\n', stderr);
}
