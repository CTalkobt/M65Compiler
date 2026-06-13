#include <string.h>
void *__builtin_memcpy(void *dest, void *src, unsigned int n) {
    return memcpy(dest, src, n);
}
