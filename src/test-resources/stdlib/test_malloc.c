// test_malloc: validate malloc/free
// Returns 0 (A=$00) on success, non-zero on failure.
//
// Requires: #pragma cc45 heap

#pragma cc45 heap

#include <stdlib.h>
#include <string.h>

int main() {
    char *p;
    char *q;

    // Basic allocation
    p = malloc(16);
    if (p == 0) return 1;

    // Write and read back
    memset(p, 0x42, 16);
    if (p[0] != 0x42) return 2;
    if (p[15] != 0x42) return 3;

    // Second allocation should not overlap
    q = malloc(16);
    if (q == 0) return 4;
    memset(q, 0x55, 16);
    // First block should be unchanged
    if (p[0] != 0x42) return 5;

    // Free and re-allocate
    free(p);
    free(q);

    p = malloc(32);
    if (p == 0) return 6;
    free(p);

    // malloc(0) returns NULL
    p = malloc(0);
    if (p != 0) return 7;

    return 0;
}
