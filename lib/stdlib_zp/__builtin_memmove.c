#include <string.h>
void *__builtin_memmove(void *dest, void *src, unsigned int n) {
    return memmove(dest, src, n);
}
