/* test_calloc_simple.c — Simple calloc() and realloc() functionality test
 *
 * Tests basic allocation and zero-initialization without stdio.
 * Returns status via exit code: 0 = success, 1 = failure
 */

#include <stdlib.h>

int main(void) {
    /* Test 1: calloc() basic allocation */
    int *arr = (int *)calloc(10, sizeof(int));
    if (arr == NULL) {
        return 1;  /* calloc failed */
    }

    /* Verify all elements are zero-initialized */
    for (int i = 0; i < 10; i++) {
        if (arr[i] != 0) {
            return 2;  /* not zero-initialized */
        }
    }

    /* Test 2: Write and read back */
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }

    for (int i = 0; i < 10; i++) {
        if (arr[i] != i * 10) {
            return 3;  /* values corrupted */
        }
    }

    /* Test 3: realloc() grow */
    int *new_arr = (int *)realloc(arr, 20 * sizeof(int));
    if (new_arr == NULL) {
        free(arr);
        return 4;  /* realloc grow failed */
    }
    arr = new_arr;

    /* Verify original data preserved */
    for (int i = 0; i < 10; i++) {
        if (arr[i] != i * 10) {
            free(arr);
            return 5;  /* original values corrupted */
        }
    }

    /* Write to new region */
    for (int i = 10; i < 20; i++) {
        arr[i] = i * 10;
    }

    /* Test 4: realloc() shrink */
    new_arr = (int *)realloc(arr, 5 * sizeof(int));
    if (new_arr == NULL) {
        free(arr);
        return 6;  /* realloc shrink failed */
    }
    arr = new_arr;

    /* Verify first 5 values still present */
    for (int i = 0; i < 5; i++) {
        if (arr[i] != i * 10) {
            free(arr);
            return 7;  /* values lost during shrink */
        }
    }

    /* Test 5: realloc(NULL, size) == malloc(size) */
    int *ptr = (int *)realloc(NULL, 3 * sizeof(int));
    if (ptr == NULL) {
        free(arr);
        return 8;  /* realloc(NULL, size) failed */
    }

    ptr[0] = 100;
    ptr[1] = 200;
    ptr[2] = 300;
    if (ptr[0] != 100 || ptr[1] != 200 || ptr[2] != 300) {
        free(arr);
        free(ptr);
        return 9;  /* values corrupted */
    }
    free(ptr);

    /* Test 6: realloc(ptr, 0) == free(ptr) */
    int *to_free = (int *)malloc(10 * sizeof(int));
    if (to_free == NULL) {
        free(arr);
        return 10;  /* malloc failed */
    }

    int *result = (int *)realloc(to_free, 0);
    if (result != NULL) {
        free(arr);
        return 11;  /* realloc(ptr, 0) didn't free */
    }

    /* Final cleanup */
    free(arr);

    return 0;  /* All tests passed */
}
