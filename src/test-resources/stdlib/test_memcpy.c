// test_memcpy: validate memcpy()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char src[8];
    char dst[8];
    int i;

    // Fill source with known pattern
    for (i = 0; i < 8; i++) src[i] = (char)(i + 1);

    // Clear destination
    for (i = 0; i < 8; i++) dst[i] = 0;

    memcpy(dst, src, 8);

    for (i = 0; i < 8; i++) {
        if (dst[i] != (char)(i + 1)) return i + 1;
    }

    // Zero-length copy should not crash
    memcpy(dst, src, 0);

    return 0;
}
