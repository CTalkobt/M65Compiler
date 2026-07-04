/* qsort.c — Quicksort for cc45 / MEGA65 */

#include <string.h>

static void swap(char *a, char *b, int size) {
    char tmp;
    int i;
    for (i = 0; i < size; i++) {
        tmp = a[i]; a[i] = b[i]; b[i] = tmp;
    }
}

typedef int (*cmpfn_t)();

cmpfn_t qs_cmp;
int qs_size;

static int do_cmp(void *a, void *b) {
    return qs_cmp(a, b);
}

/* Signed greater-than-zero: checks high bit for sign */
static int cmp_positive(void *a, void *b) {
    int v = do_cmp(a, b);
    /* Use arithmetic: positive if non-zero and high byte < 0x80 */
    unsigned int uv = (unsigned int)v;
    if (uv == 0) return 0;
    return (uv >> 8) < 128 ? 1 : 0;
}

/* Signed less-than-or-equal-to-zero */
static int cmp_not_positive(void *a, void *b) {
    return !cmp_positive(a, b);
}

static void qs_impl(char *arr, int nmemb) {
    int i, j;
    char *pivot;
    int size = qs_size;

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
        int mid = nmemb / 2;
        int last = nmemb - 1;
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

void qsort(void *base, int nmemb, int size, int (*cmpfunc)()) {
    qs_cmp = cmpfunc;
    qs_size = size;
    qs_impl((char *)base, nmemb);
}
