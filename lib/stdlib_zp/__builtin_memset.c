#include <string.h>
void *__builtin_memset(void *dest, int val, unsigned int n) {
    return memset(dest, val, n);
}
