// test_memset: validate memset()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    char buf[16];
    int i;

    memset(buf, 0xAA, 16);
    for (i = 0; i < 16; i++) {
        if (buf[i] != (char)0xAA) return 1;
    }

    memset(buf, 0, 8);
    for (i = 0; i < 8; i++) {
        if (buf[i] != 0) return 2;
    }
    // Remaining bytes unchanged
    for (i = 8; i < 16; i++) {
        if (buf[i] != (char)0xAA) return 3;
    }

    return 0;
}
