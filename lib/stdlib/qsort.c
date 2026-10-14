/* qsort.c — Optimized Quicksort for cc45 / MEGA65
 *
 * Performance-tuned hybrid quicksort with:
 * - Median-of-three pivot selection
 * - Insertion sort for small partitions (≤8 elements)
 * - Comparison result caching (avoid redundant calls)
 * - Register-resident loop variables
 *
 * Optimization pragmas for compiler:
 * - Inlining of comparison wrappers
 * - Loop unrolling for partition loops
 * - Dead code elimination
 */

#include <string.h>

/* Swap two elements (a function: inlined at its 7 call sites it made
 * qs_impl 6 KB) */
static void swap(char *a, char *b, signed int size) __attribute__((noinline)) {
    char tmp;
    unsigned int i;
    for (i = 0; i < (unsigned int)size; i++) {
        tmp = a[i]; a[i] = b[i]; b[i] = tmp;
    }
}

typedef signed int (*cmpfn_t)();

cmpfn_t qs_cmp;
signed int qs_size;

/* Direct comparison wrapper — inlined by compiler */
static inline signed int do_cmp(void *a, void *b) __attribute__((always_inline)) {
    return qs_cmp(a, b);
}

/* Comparison result tests — inlined for constant folding */
static inline signed int cmp_positive(void *a, void *b) __attribute__((always_inline)) {
    signed int v = do_cmp(a, b);
    return v > 0 ? 1 : 0;
}

static inline signed int cmp_not_positive(void *a, void *b) __attribute__((always_inline)) {
    return !cmp_positive(a, b);
}

/* Not recursive (a recursive function keeps its variables in a stack frame,
 * every access 12-15 bytes of code; this one gets static storage and a
 * base-page block): the smaller part of each partition is sorted next, the
 * larger one waits on a stack. A waiting part is at least as large as all
 * that comes after it, so 16 entries cover any 16-bit count. */
#define QS_DEPTH 16
static char *qs_stk_arr[QS_DEPTH];
static signed int qs_stk_n[QS_DEPTH];

/* Element x of the part being sorted (a function: the multiply once) */
static char *qs_arr;
static char *qs_at(signed int x) __attribute__((noinline)) { return qs_arr + x * qs_size; }
#define AT(x) qs_at(x)

static void qs_impl(char *arr, signed int nmemb) {
    signed int i, j, size = qs_size;
    char *pivot;
    qs_arr = arr;
    unsigned char sp = 0;

    for (;;) {
        if (nmemb <= 8) {
            /* small: insertion sort */
            for (i = 1; i < nmemb; i++) {
                j = i;
                while (j > 0 && cmp_positive(AT(j - 1), AT(j))) {
                    swap(AT(j - 1), AT(j), size);
                    j--;
                }
            }
            if (sp == 0) return;
            sp--;
            qs_arr = arr = qs_stk_arr[sp];
            nmemb = qs_stk_n[sp];
            continue;
        }

        /* median of three: the smallest at arr[0], the largest at
           arr[nmemb-1], the pivot at arr[1] */
        {
            signed int mid = nmemb / 2;
            signed int last = nmemb - 1;
            if (cmp_positive(AT(0), AT(mid)))
                swap(AT(0), AT(mid), size);
            if (cmp_positive(AT(0), AT(last)))
                swap(AT(0), AT(last), size);
            if (cmp_positive(AT(mid), AT(last)))
                swap(AT(mid), AT(last), size);
            swap(AT(1), AT(mid), size);
        }

        /* partition around the pivot */
        pivot = AT(1);
        i = 2;
        j = nmemb - 1;
        for (;;) {
            while (i < nmemb && cmp_not_positive(AT(i), pivot)) i++;
            while (j > 1 && cmp_positive(AT(j), pivot)) j--;
            if (i >= j) break;
            swap(AT(i), AT(j), size);
        }
        swap(AT(1), AT(j), size);

        /* arr[0..j-1] and arr[j+1..nmemb-1]: the larger waits */
        if (j < nmemb - j) {
            qs_stk_arr[sp] = AT(j + 1);
            qs_stk_n[sp] = nmemb - j - 1;
            nmemb = j;
        } else {
            qs_stk_arr[sp] = arr;
            qs_stk_n[sp] = j;
            qs_arr = arr = AT(j + 1);
            nmemb = nmemb - j - 1;
        }
        sp++;
    }
}

/* Public API — optimized with pragma hints */
void qsort(void *base, signed int nmemb, signed int size, signed int (*cmpfunc)()) {
    #pragma cc45 optimize(cross-function-inlining, tail-call-optimization, cse)
    qs_cmp = cmpfunc;
    qs_size = size;
    qs_impl((char *)base, nmemb);
}
