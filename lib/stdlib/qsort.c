/* qsort.c — Quicksort for cc45 / MEGA65 */

#include <string.h>

int (*__qsort_cmp)();
int __qsort_size;

static void swap(char *a, char *b, int size) {
    char tmp;
    int i;
    for (i = 0; i < size; i++) {
        tmp = a[i];
        a[i] = b[i];
        b[i] = tmp;
    }
}

static void qs_impl(char *arr, int nmemb) {
    int i, j;
    char *pivot;
    int size = __qsort_size;

    if (nmemb <= 1) return;

    /* Insertion sort for small arrays */
    if (nmemb <= 8) {
        for (i = 1; i < nmemb; i++) {
            j = i;
            while (j > 0 && __qsort_cmp(arr + (j - 1) * size, arr + j * size) > 0) {
                swap(arr + (j - 1) * size, arr + j * size, size);
                j--;
            }
        }
        return;
    }

    /* Median-of-three pivot */
    {
        int mid = nmemb / 2;
        int last = nmemb - 1;
        if (__qsort_cmp(arr, arr + mid * size) > 0)
            swap(arr, arr + mid * size, size);
        if (__qsort_cmp(arr, arr + last * size) > 0)
            swap(arr, arr + last * size, size);
        if (__qsort_cmp(arr + mid * size, arr + last * size) > 0)
            swap(arr + mid * size, arr + last * size, size);
        swap(arr + size, arr + mid * size, size);
    }

    pivot = arr + size;
    i = 2;
    j = nmemb - 1;

    while (1) {
        while (i < nmemb && __qsort_cmp(arr + i * size, pivot) <= 0) i++;
        while (j > 1 && __qsort_cmp(arr + j * size, pivot) > 0) j--;
        if (i >= j) break;
        swap(arr + i * size, arr + j * size, size);
    }

    swap(arr + size, arr + j * size, size);

    if (j < nmemb - j) {
        qs_impl(arr, j);
        qs_impl(arr + (j + 1) * size, nmemb - j - 1);
    } else {
        qs_impl(arr + (j + 1) * size, nmemb - j - 1);
        qs_impl(arr, j);
    }
}

void qsort(void *base, int nmemb, int size, int (*cmpfunc)()) {
    __qsort_cmp = cmpfunc;
    __qsort_size = size;
    qs_impl((char *)base, nmemb);
}
