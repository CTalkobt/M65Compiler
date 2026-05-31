/**
 * Pass 4 (seq-extract) Optimization Test
 *
 * Tests that Pass 4 correctly extracts and shares duplicate instruction sequences
 * without introducing code corruption (issue #89 fix verification).
 *
 * Compile normally: cc45 test_pass4_seq_extract.c -o normal.prg
 * Compile with optimization: ca45 test_pass4_seq_extract.s --experimental
 *
 * Expected: Optimized version should be smaller without any BRK instructions
 * in unexpected locations.
 */

#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

/**
 * This function has multiple duplicate call sequences that Pass 4 should extract.
 * Each loop iteration has three identical JSR sequences.
 */
int multiply_with_duplicates(int a, int b) {
    int result = 0;

    for (int i = 0; i < b; i++) {
        result = add(result, a);     /* Sequence 1 */
        result = add(result, a);     /* Sequence 2 */
        result = add(result, a);     /* Sequence 3 */
    }

    return result;
}

/**
 * Similar pattern to trigger seq-extract
 */
int accumulate(int *arr, int n) {
    int sum = 0;

    for (int i = 0; i < n; i++) {
        sum = add(sum, arr[0]);       /* Sequence 1 */
        sum = add(sum, arr[1]);       /* Sequence 2 */
        sum = add(sum, arr[2]);       /* Sequence 3 */
    }

    return sum;
}

int main(void) {
    /* Test multiply_with_duplicates */
    int result1 = multiply_with_duplicates(5, 3);

    /* Test accumulate */
    int arr[3] = {10, 20, 30};
    int result2 = accumulate(arr, 2);

    printf("Result 1: %d\n", result1);
    printf("Result 2: %d\n", result2);

    /* Verify correctness */
    if (result1 != 45) return 1;  /* 5*3*3 = 45 */
    if (result2 != 180) return 1; /* (10+20+30)*2 = 120 */

    return 0;
}
