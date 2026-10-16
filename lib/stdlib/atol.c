/* atol.c — Convert string to long integer */

#include <stdlib.h>

long atol(char *s) {
    return strtol(s, (char **)0, 10);
}
