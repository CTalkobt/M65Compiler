/* qsort.c — Quicksort for cc45 / MEGA65 */

#include <string.h>

static void swap(char *a, char *b, signed int size) {
    char tmp;
    signed int i;
    for (i = 0; i < size; i++) {
        tmp = a[i]; a[i] = b[i]; b[i] = tmp;
    }
}

typedef signed int (*cmpfn_t)();

cmpfn_t qs_cmp;
signed int qs_size;

static signed int do_cmp(void *a, void *b) {
    return qs_cmp(a, b);
}

/* Signed greater-than-zero: compare result > 0 */
static signed int cmp_positive(void *a, void *b) {
    signed int v = do_cmp(a, b);
    return v > 0 ? 1 : 0;
}

/* Signed less-than-or-equal-to-zero */
static signed int cmp_not_positive(void *a, void *b) {
    return !cmp_positive(a, b);
}

static void qs_impl(char *arr, signed int nmemb) {
    signed int i, j;
    char *pivot;
    signed int size = qs_size;

    if (nmemb <= 1) return;

    if (nmemb <= 8) {
        for (i = 1; i < nmemb; i++) {
            j = i;
            while (j > 0 && cmp_positive(arr + (j - 1) * size, arr + j * size)) {
                swap(arr + (j - 1) * size, arr + j * size, size);
                j--;
            }
        }
        return;
    }

    {
        signed int mid = nmemb / 2;
        signed int last = nmemb - 1;
        if (cmp_positive(arr, arr + mid * size))
            swap(arr, arr + mid * size, size);
        if (cmp_positive(arr, arr + last * size))
            swap(arr, arr + last * size, size);
        if (cmp_positive(arr + mid * size, arr + last * size))
            swap(arr + mid * size, arr + last * size, size);
        swap(arr + size, arr + mid * size, size);
    }

    pivot = arr + size;
    i = 2;
    j = nmemb - 1;

    while (1) {
        while (i < nmemb && cmp_not_positive(arr + i * size, pivot)) i++;
        while (j > 1 && cmp_positive(arr + j * size, pivot)) j--;
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

void qsort(void *base, signed int nmemb, signed int size, signed int (*cmpfunc)()) {
    qs_cmp = cmpfunc;
    qs_size = size;
    qs_impl((char *)base, nmemb);
}
