#include <string.h>
int __builtin_memcmp(const void *s1, const void *s2, unsigned int n) {
    return memcmp(s1, s2, n);
}
