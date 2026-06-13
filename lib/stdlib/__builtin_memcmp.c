#include <string.h>
int __builtin_memcmp(void *s1, void *s2, unsigned int n) {
    return memcmp(s1, s2, n);
}
