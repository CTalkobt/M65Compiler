// Example: memset/memcpy/memcmp — validate memory operations
// Uses c45.lib string.h implementation.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

char src[8];
char dst[8];

int main() {
    // memset: fill with a byte
    memset(src, 0x42, 4);
    if (src[0] != 0x42) return 1;
    if (src[3] != 0x42) return 2;

    // memcpy: copy bytes
    memset(dst, 0, 8);
    memcpy(dst, src, 4);
    if (dst[0] != 0x42) return 3;
    if (dst[3] != 0x42) return 4;

    // memcmp: compare equal regions
    if (memcmp(src, dst, 4) != 0) return 5;

    // memcmp: compare after modification
    dst[2] = 0x41;
    if (memcmp(src, dst, 4) <= 0) return 6;

    return 0;
}
