#include <string.h>
char *__builtin_strcpy(char *restrict dest, const char *restrict src) {
    return strcpy(dest, src);
}
