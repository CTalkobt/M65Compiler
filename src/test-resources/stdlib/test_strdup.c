// test_strdup: validate string duplication
// Returns 0 (A=$00) on success, non-zero on failure.
//
// Requires: #pragma cc45 heap

#pragma cc45 heap

#include <stdlib.h>
#include <string.h>

int main() {
    char *dup;
    const char *original = "hello world";
    const char *empty = "";

    // Test 1: Basic duplication
    dup = strdup(original);
    if (dup == 0) return 1;  // malloc failed
    if (strcmp(dup, original) != 0) return 2;

    // Test 2: Verify independence (modify copy doesn't affect original)
    dup[0] = 'H';
    if (original[0] != 'h') return 3;  // Original should be unchanged

    // Test 3: Verify they are different pointers
    if ((void *)dup == (void *)original) return 4;

    free(dup);

    // Test 4: Duplicate empty string
    dup = strdup(empty);
    if (dup == 0) return 5;
    if (strcmp(dup, "") != 0) return 6;
    free(dup);

    // Test 5: Duplicate string with special characters
    const char *special = "abc\n\t123";
    dup = strdup(special);
    if (dup == 0) return 7;
    if (strcmp(dup, special) != 0) return 8;
    free(dup);

    // Test 6: Multiple duplicates can coexist
    char *dup1 = strdup("first");
    char *dup2 = strdup("second");
    if (dup1 == 0 || dup2 == 0) return 9;
    if (strcmp(dup1, "first") != 0) return 10;
    if (strcmp(dup2, "second") != 0) return 11;
    // Modify one, other should be unaffected
    dup1[0] = 'F';
    if (strcmp(dup2, "second") != 0) return 12;
    free(dup1);
    free(dup2);

    return 0;  // Success
}
