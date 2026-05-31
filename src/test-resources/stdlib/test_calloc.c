// test_calloc: validate calloc() - zeroed allocation
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>
#include <string.h>

int main() {
    char *p;
    int i;

    // Test 1: Allocate and verify zeroed memory
    p = calloc(10, 1);
    if (p == NULL) return 1;
    for (i = 0; i < 10; i++) {
        if (p[i] != 0) return 2;
    }
    free(p);

    // Test 2: Larger allocation
    int *pint = (int *)calloc(4, 4); // 4 ints
    if (pint == NULL) return 3;
    for (i = 0; i < 4; i++) {
        if (pint[i] != 0) return 4;
    }
    free(pint);

    // Test 3: Single element
    p = calloc(1, 1);
    if (p == NULL) return 5;
    if (*p != 0) return 6;
    free(p);

    // Test 4: Large element size
    p = calloc(2, 256);
    if (p == NULL) return 7;
    for (i = 0; i < 256; i++) {
        if (p[i] != 0) return 8;
    }
    free(p);

    // Test 5: Write to allocated memory
    p = calloc(5, 1);
    if (p == NULL) return 9;
    p[0] = 'A';
    p[1] = 'B';
    p[2] = 'C';
    if (p[0] != 'A' || p[1] != 'B' || p[2] != 'C') return 10;
    if (p[3] != 0 || p[4] != 0) return 11; // Rest should still be zero
    free(p);

    // Test 6: Multiple allocations
    char *p1 = calloc(3, 1);
    char *p2 = calloc(3, 1);
    if (p1 == NULL || p2 == NULL) return 12;
    if (p1 == p2) return 13; // Should be different addresses
    p1[0] = 'X';
    if (p2[0] != 0) return 14; // p2 still zero
    free(p1);
    free(p2);

    // Test 7: Zero elements (edge case - may return NULL or valid pointer)
    p = calloc(0, 10);
    // Accept either NULL or a valid pointer

    // Test 8: Zero size (edge case)
    p = calloc(10, 0);
    // Accept either NULL or a valid pointer

    // Test 9: Array of structs simulation
    struct point {
        int x;
        int y;
    };
    struct point *points = (struct point *)calloc(3, sizeof(struct point));
    if (points == NULL) return 15;
    for (i = 0; i < 3; i++) {
        if (points[i].x != 0 || points[i].y != 0) return 16;
    }
    points[0].x = 10;
    points[1].y = 20;
    if (points[0].x != 10 || points[0].y != 0) return 17;
    if (points[1].x != 0 || points[1].y != 20) return 18;
    free(points);

    // Test 10: Sequential allocations
    char *a = calloc(2, 1);
    char *b = calloc(2, 1);
    char *c = calloc(2, 1);
    if (a == NULL || b == NULL || c == NULL) return 19;
    a[0] = 1;
    b[0] = 2;
    c[0] = 3;
    if (a[0] != 1 || b[0] != 2 || c[0] != 3) return 20;
    free(a);
    free(b);
    free(c);

    return 0;
}
