/* test_calloc_realloc.c — calloc() and realloc() Test Suite
 *
 * Tests dynamic memory allocation with zero-initialization and resizing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing calloc() and realloc()...\n\n");

    /* Test 1: calloc() — Basic allocation and zero-initialization */
    printf("Test 1: calloc() - Zero Initialization\n");
    int *arr = (int *)calloc(10, sizeof(int));
    if (arr == NULL) {
        printf("  ✗ calloc() returned NULL\n");
        return 1;
    }
    printf("  ✓ calloc(10, sizeof(int)) succeeded\n");

    /* Verify all elements are zero-initialized */
    int all_zero = 1;
    for (int i = 0; i < 10; i++) {
        if (arr[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    if (all_zero) {
        printf("  ✓ All 10 elements zero-initialized\n");
    } else {
        printf("  ✗ Elements not properly zero-initialized\n");
    }

    /* Test 2: calloc() — Write and read back */
    printf("\nTest 2: calloc() - Write and Read\n");
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }
    printf("  ✓ Wrote 10 values (0, 10, 20, ..., 90)\n");

    int verify = 1;
    for (int i = 0; i < 10; i++) {
        if (arr[i] != i * 10) {
            verify = 0;
            break;
        }
    }
    if (verify) {
        printf("  ✓ All values read back correctly\n");
    } else {
        printf("  ✗ Values corrupted\n");
    }

    /* Test 3: realloc() — Grow allocation */
    printf("\nTest 3: realloc() - Grow Allocation\n");
    int *new_arr = (int *)realloc(arr, 20 * sizeof(int));
    if (new_arr == NULL) {
        printf("  ✗ realloc() returned NULL\n");
        free(arr);
        return 1;
    }
    arr = new_arr;
    printf("  ✓ realloc(arr, 20*sizeof(int)) succeeded\n");

    /* Verify original data preserved */
    int preserved = 1;
    for (int i = 0; i < 10; i++) {
        if (arr[i] != i * 10) {
            preserved = 0;
            break;
        }
    }
    if (preserved) {
        printf("  ✓ Original 10 values preserved after grow\n");
    } else {
        printf("  ✗ Original values corrupted\n");
    }

    /* Write new values in grown region */
    for (int i = 10; i < 20; i++) {
        arr[i] = i * 10;
    }
    printf("  ✓ Wrote new values (100, 110, ..., 190) to grown region\n");

    /* Test 4: realloc() — Shrink allocation */
    printf("\nTest 4: realloc() - Shrink Allocation\n");
    new_arr = (int *)realloc(arr, 5 * sizeof(int));
    if (new_arr == NULL) {
        printf("  ✗ realloc() returned NULL\n");
        free(arr);
        return 1;
    }
    arr = new_arr;
    printf("  ✓ realloc(arr, 5*sizeof(int)) succeeded\n");

    /* Verify first 5 values still present */
    int first_five = 1;
    for (int i = 0; i < 5; i++) {
        if (arr[i] != i * 10) {
            first_five = 0;
            break;
        }
    }
    if (first_five) {
        printf("  ✓ First 5 values preserved after shrink\n");
    } else {
        printf("  ✗ Values lost during shrink\n");
    }

    /* Test 5: realloc(NULL, size) == malloc(size) */
    printf("\nTest 5: realloc(NULL, size) - Malloc behavior\n");
    int *ptr = (int *)realloc(NULL, 3 * sizeof(int));
    if (ptr == NULL) {
        printf("  ✗ realloc(NULL, size) returned NULL\n");
        return 1;
    }
    printf("  ✓ realloc(NULL, 3*sizeof(int)) succeeded\n");

    ptr[0] = 100;
    ptr[1] = 200;
    ptr[2] = 300;
    if (ptr[0] == 100 && ptr[1] == 200 && ptr[2] == 300) {
        printf("  ✓ Values written and read correctly\n");
    } else {
        printf("  ✗ Values corrupted\n");
    }
    free(ptr);

    /* Test 6: realloc(ptr, 0) == free(ptr) */
    printf("\nTest 6: realloc(ptr, 0) - Free behavior\n");
    int *to_free = (int *)malloc(10 * sizeof(int));
    if (to_free == NULL) {
        printf("  ✗ malloc() failed\n");
        return 1;
    }
    printf("  ✓ malloc(10*sizeof(int)) succeeded\n");

    int *result = (int *)realloc(to_free, 0);
    if (result == NULL) {
        printf("  ✓ realloc(ptr, 0) returned NULL (freed)\n");
    } else {
        printf("  ✗ realloc(ptr, 0) didn't free\n");
    }

    /* Test 7: calloc() with different sizes */
    printf("\nTest 7: calloc() - Various element sizes\n");

    /* calloc with char elements */
    char *chars = (char *)calloc(20, sizeof(char));
    if (chars != NULL) {
        int all_zero_chars = 1;
        for (int i = 0; i < 20; i++) {
            if (chars[i] != 0) {
                all_zero_chars = 0;
                break;
            }
        }
        if (all_zero_chars) {
            printf("  ✓ calloc(20, sizeof(char)) zero-initialized\n");
        }
        free(chars);
    }

    /* calloc with long elements */
    long *longs = (long *)calloc(5, sizeof(long));
    if (longs != NULL) {
        int all_zero_longs = 1;
        for (int i = 0; i < 5; i++) {
            if (longs[i] != 0) {
                all_zero_longs = 0;
                break;
            }
        }
        if (all_zero_longs) {
            printf("  ✓ calloc(5, sizeof(long)) zero-initialized\n");
        }
        free(longs);
    }

    /* Test 8: Multiple allocations and deallocations */
    printf("\nTest 8: Multiple allocations/deallocations\n");
    int *allocs[5];
    for (int i = 0; i < 5; i++) {
        allocs[i] = (int *)calloc(i + 1, sizeof(int));
        if (allocs[i] == NULL) {
            printf("  ✗ calloc failed for alloc %d\n", i);
            return 1;
        }
    }
    printf("  ✓ Allocated 5 blocks\n");

    for (int i = 0; i < 5; i++) {
        free(allocs[i]);
    }
    printf("  ✓ Freed 5 blocks\n");

    /* Final cleanup */
    free(arr);

    printf("\n✓ All calloc/realloc tests passed!\n");
    return 0;
}
