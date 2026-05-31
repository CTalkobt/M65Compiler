// test_realloc: validate realloc() - dynamic memory resizing
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>
#include <string.h>

int main() {
    char *p;
    int i;

    // Test 1: Grow allocation
    p = malloc(5);
    if (p == NULL) return 1;
    strcpy(p, "hi");
    p = realloc(p, 10);
    if (p == NULL) return 2;
    if (strcmp(p, "hi") != 0) return 3; // Data preserved
    free(p);

    // Test 2: Shrink allocation
    p = malloc(20);
    if (p == NULL) return 4;
    strcpy(p, "hello");
    p = realloc(p, 10);
    if (p == NULL) return 5;
    if (strcmp(p, "hello") != 0) return 6;
    free(p);

    // Test 3: Realloc with NULL (like malloc)
    p = NULL;
    p = realloc(p, 5);
    if (p == NULL) return 7;
    strcpy(p, "test");
    free(p);

    // Test 4: Realloc to zero size (like free, may return NULL)
    p = malloc(10);
    if (p == NULL) return 8;
    strcpy(p, "data");
    p = realloc(p, 0);
    // p may be NULL after this

    // Test 5: Data preservation with multiple reallocations
    p = malloc(4);
    if (p == NULL) return 9;
    p[0] = 1; p[1] = 2; p[2] = 3;
    p = realloc(p, 8);
    if (p == NULL) return 10;
    if (p[0] != 1 || p[1] != 2 || p[2] != 3) return 11;
    p[3] = 4; p[4] = 5;
    p = realloc(p, 12);
    if (p == NULL) return 12;
    if (p[0] != 1 || p[1] != 2 || p[2] != 3 || p[3] != 4 || p[4] != 5) return 13;
    free(p);

    // Test 6: Multiple independent reallocations
    char *p1 = malloc(5);
    char *p2 = malloc(5);
    if (p1 == NULL || p2 == NULL) return 14;
    strcpy(p1, "AAA");
    strcpy(p2, "BBB");
    p1 = realloc(p1, 10);
    if (p1 == NULL) return 15;
    if (strcmp(p1, "AAA") != 0) return 16;
    if (strcmp(p2, "BBB") != 0) return 17; // p2 unaffected
    free(p1);
    free(p2);

    // Test 7: Grow large allocation
    p = malloc(10);
    if (p == NULL) return 18;
    for (i = 0; i < 10; i++) p[i] = i;
    p = realloc(p, 100);
    if (p == NULL) return 19;
    for (i = 0; i < 10; i++) {
        if (p[i] != i) return 20;
    }
    free(p);

    // Test 8: Realloc same size (should work)
    p = malloc(8);
    if (p == NULL) return 21;
    strcpy(p, "data");
    p = realloc(p, 8);
    if (p == NULL) return 22;
    if (strcmp(p, "data") != 0) return 23;
    free(p);

    // Test 9: Shrink to very small
    p = malloc(50);
    if (p == NULL) return 24;
    strcpy(p, "X");
    p = realloc(p, 2);
    if (p == NULL) return 25;
    if (strcmp(p, "X") != 0) return 26;
    free(p);

    // Test 10: Complex realloc sequence
    p = malloc(4);
    if (p == NULL) return 27;
    p[0] = 'A';
    p = realloc(p, 8);
    if (p == NULL) return 28;
    p[1] = 'B';
    p = realloc(p, 16);
    if (p == NULL) return 29;
    p[2] = 'C';
    p = realloc(p, 8);
    if (p == NULL) return 30;
    if (p[0] != 'A' || p[1] != 'B' || p[2] != 'C') return 31;
    free(p);

    return 0;
}
