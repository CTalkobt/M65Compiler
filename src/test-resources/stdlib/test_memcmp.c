// test_memcmp: validate memcmp()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    // Test 1: Equal memory blocks
    char a[4] = {1, 2, 3, 4};
    char b[4] = {1, 2, 3, 4};
    if (memcmp(a, b, 4) != 0) return 1;

    // Test 2: Different memory blocks (first is less)
    char c[4] = {1, 2, 3, 4};
    char d[4] = {1, 2, 4, 4};
    int cmp = memcmp(c, d, 4);
    if (cmp >= 0) return 2; // c < d, so should be negative

    // Test 3: Different memory blocks (first is greater)
    char e[4] = {1, 2, 5, 4};
    char f[4] = {1, 2, 4, 4};
    cmp = memcmp(e, f, 4);
    if (cmp <= 0) return 3; // e > f, so should be positive

    // Test 4: Zero-length comparison (should be equal)
    if (memcmp(a, b, 0) != 0) return 4;

    // Test 5: Partial comparison (first part equal)
    char g[4] = {1, 2, 3, 4};
    char h[4] = {1, 2, 9, 9};
    if (memcmp(g, h, 2) != 0) return 5; // First 2 bytes are equal

    // Test 6: Partial comparison (different)
    if (memcmp(g, h, 3) == 0) return 6; // Third byte differs

    // Test 7: Compare strings as bytes
    char *s1 = "hello";
    char *s2 = "hello";
    if (memcmp(s1, s2, 5) != 0) return 7;

    // Test 8: Compare with different strings
    char *s3 = "hello";
    char *s4 = "world";
    cmp = memcmp(s3, s4, 5);
    if (cmp == 0) return 8; // Should be different

    // Test 9: Compare first byte only
    if (memcmp(s3, s4, 1) == 0) return 9; // 'h' != 'w'

    // Test 10: Single byte comparison
    char x = 42;
    char y = 42;
    if (memcmp(&x, &y, 1) != 0) return 10;

    // Test 11: Single byte different
    char z = 43;
    cmp = memcmp(&x, &z, 1);
    if (cmp >= 0) return 11; // 42 < 43

    // Test 12: Large array partial comparison
    char big1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    char big2[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    if (memcmp(big1, big2, 10) != 0) return 12;

    return 0;
}
