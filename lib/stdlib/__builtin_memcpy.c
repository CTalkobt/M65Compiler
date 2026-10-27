#include <string.h>
void *__builtin_memcpy(void *restrict dest, const void *restrict src, unsigned int n) {
    return memcpy(dest, src, n);
}
