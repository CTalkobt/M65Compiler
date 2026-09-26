// test_calloc: validate calloc zero-initialization
// Returns 0 (A=$00) on success, non-zero on failure.
//
// Requires: #pragma cc45 heap

#pragma cc45 heap

#include <stdlib.h>
#include <string.h>

int main() {
    char *p;
    char *q;

    // Test 1: Basic allocation and zero-initialization
    p = calloc(16, 1);
    if (p == 0) return 1;

    // Verify all bytes are zero
    if (p[0] != 0) return 2;
    if (p[15] != 0) return 3;

    // Test 2: calloc with multiplication
    q = calloc(4, 4);  // 4 * 4 = 16 bytes
    if (q == 0) return 4;
    if (q[0] != 0) return 5;
    if (q[15] != 0) return 6;

    // Test 3: Verify blocks don't overlap
    p[0] = 0x42;
    q[0] = 0x55;
    if (p[0] != 0x42) return 7;  // p should still be 0x42
    if (q[0] != 0x55) return 8;  // q should be 0x55

    // Test 4: calloc(0, N) returns NULL
    char *null_test = calloc(0, 100);
    if (null_test != 0) return 9;

    // Test 5: calloc(N, 0) returns NULL
    null_test = calloc(100, 0);
    if (null_test != 0) return 10;

    // Test 6: Multiple allocations with zero-init
    free(p);
    free(q);

    p = calloc(8, 1);
    if (p == 0) return 11;
    memset(p, 0xAA, 8);
    if (p[0] != 0xAA) return 12;

    free(p);

    return 0;  // Success
}
