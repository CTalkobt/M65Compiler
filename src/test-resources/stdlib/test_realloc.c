// test_realloc: validate realloc behavior
// Returns 0 (A=$00) on success, non-zero on failure.
//
// Requires: #pragma cc45 heap

#pragma cc45 heap

#include <stdlib.h>
#include <string.h>

int main() {
    char *p;
    char *q;

    // Test 1: realloc(NULL, size) behaves like malloc
    p = realloc(NULL, 16);
    if (p == 0) return 1;
    p[0] = 0x42;
    if (p[0] != 0x42) return 2;

    // Test 2: realloc to larger size
    q = realloc(p, 32);
    if (q == 0) return 3;
    // Verify data was copied
    if (q[0] != 0x42) return 4;

    // Test 3: Fill with pattern, realloc larger, verify copy
    memset(q, 0x55, 16);
    p = realloc(q, 64);
    if (p == 0) return 5;
    // First 16 bytes should still have 0x55 pattern
    if (p[0] != 0x55) return 6;
    if (p[15] != 0x55) return 7;

    // Test 4: realloc to smaller size
    memset(p, 0xAA, 64);
    q = realloc(p, 8);
    if (q == 0) return 8;
    // First 8 bytes should still be 0xAA
    if (q[0] != 0xAA) return 9;
    if (q[7] != 0xAA) return 10;

    // Test 5: realloc(ptr, 0) frees and returns NULL
    p = realloc(q, 0);
    if (p != 0) return 11;

    // Test 6: Allocate after free
    p = malloc(16);
    if (p == 0) return 12;
    p[0] = 0x77;
    if (p[0] != 0x77) return 13;
    free(p);

    return 0;  // Success
}
