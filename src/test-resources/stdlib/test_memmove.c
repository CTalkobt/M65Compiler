// test_memmove: validate memmove() with overlapping regions
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Non-overlapping forward copy
    char buf1[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memmove(buf1 + 4, buf1, 4);
    if (buf1[4] != 1 || buf1[5] != 2 || buf1[6] != 3 || buf1[7] != 4) return 1;

    // Test 2: Non-overlapping backward copy
    char buf2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memmove(buf2, buf2 + 4, 4);
    if (buf2[0] != 5 || buf2[1] != 6 || buf2[2] != 7 || buf2[3] != 8) return 2;

    // Test 3: Overlapping forward (dest after src)
    // Source: [A, B, C, D, _, _, _, _]
    // After move: [A, B, C, D, A, B, C, D]
    char buf3[8] = {10, 20, 30, 40, 0, 0, 0, 0};
    memmove(buf3 + 4, buf3, 4);
    if (buf3[4] != 10 || buf3[5] != 20 || buf3[6] != 30 || buf3[7] != 40) return 3;

    // Test 4: Overlapping backward (dest before src)
    // Source: [_, _, _, _, A, B, C, D]
    // After move: [A, B, C, D, A, B, C, D]
    char buf4[8] = {0, 0, 0, 0, 50, 60, 70, 80};
    memmove(buf4, buf4 + 4, 4);
    if (buf4[0] != 50 || buf4[1] != 60 || buf4[2] != 70 || buf4[3] != 80) return 4;

    // Test 5: Heavily overlapping (copy with 1 byte offset)
    char buf5[6] = {1, 2, 3, 4, 5, 6};
    memmove(buf5 + 1, buf5, 5);
    if (buf5[1] != 1 || buf5[2] != 2 || buf5[3] != 3 || buf5[4] != 4 || buf5[5] != 5) return 5;

    // Test 6: Zero-length move (should do nothing)
    char buf6[4] = {1, 2, 3, 4};
    memmove(buf6 + 2, buf6, 0);
    if (buf6[0] != 1 || buf6[1] != 2 || buf6[2] != 3 || buf6[3] != 4) return 6;

    // Test 7: Single byte move
    char buf7[3] = {1, 2, 3};
    memmove(buf7 + 1, buf7, 1);
    if (buf7[1] != 1) return 7;

    // Test 8: Move to same location (should be identity)
    char buf8[4] = {10, 20, 30, 40};
    memmove(buf8, buf8, 4);
    if (buf8[0] != 10 || buf8[1] != 20 || buf8[2] != 30 || buf8[3] != 40) return 8;

    // Test 9: Partial overlap forward
    char buf9[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memmove(buf9 + 2, buf9, 5);
    if (buf9[2] != 1 || buf9[3] != 2 || buf9[4] != 3 || buf9[5] != 4 || buf9[6] != 5) return 9;

    // Test 10: Partial overlap backward
    char buf10[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memmove(buf10, buf10 + 2, 5);
    if (buf10[0] != 3 || buf10[1] != 4 || buf10[2] != 5 || buf10[3] != 6 || buf10[4] != 7) return 10;

    return 0;
}
