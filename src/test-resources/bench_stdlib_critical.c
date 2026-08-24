/* bench_stdlib_critical.c — Standard Library Performance Benchmarks
 *
 * Measures performance of critical-path stdlib functions on MEGA65.
 * Reports cycle counts and throughput metrics.
 *
 * Functions tested:
 * 1. malloc/free — Memory allocation
 * 2. memcpy/memset — Memory operations
 * 3. qsort/bsearch — Sorting/searching
 * 4. strcmp/strlen — String operations
 * 5. rand — Random number generation
 */

#include <stdlib.h>
#include <string.h>

/* Timer support — assumes MEGA65 jiffy clock at $D711 */
static unsigned char read_jiffy(void) {
    return *(volatile unsigned char *)0xD711;
}

/* Simple cycle counter wrapper (jiffy = 1/60 second ≈ 16667 cycles) */
typedef struct {
    unsigned char start;
    unsigned char end;
    unsigned int cycles;
} bench_timer_t;

static void bench_start(bench_timer_t *t) {
    t->start = read_jiffy();
}

static void bench_end(bench_timer_t *t) {
    t->end = read_jiffy();
    t->cycles = (unsigned int)(t->end - t->start) * 16667;  /* Estimate */
}

/* ========================================================================
 * Benchmark 1: malloc/free rapid allocation cycle
 * ======================================================================== */

void bench_malloc_rapid(void) {
    bench_timer_t timer;
    int iterations = 100;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        void *ptrs[10];
        int j;
        for (j = 0; j < 10; j++) {
            ptrs[j] = malloc(32);
        }
        for (j = 0; j < 10; j++) {
            free(ptrs[j]);
        }
    }
    bench_end(&timer);

    /* Result: cycles per alloc/free pair
     * Expected: 100-200 cycles per pair on optimized path */
}

/* ========================================================================
 * Benchmark 2: memcpy throughput
 * ======================================================================== */

void bench_memcpy_throughput(void) {
    bench_timer_t timer;
    char src[256];
    char dst[256];
    int iterations = 100;
    int i;

    /* Initialize source */
    for (i = 0; i < 256; i++) {
        src[i] = i & 0xFF;
    }

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        memcpy(dst, src, 256);
    }
    bench_end(&timer);

    /* Result: cycles per byte copied
     * Expected: 1-2 cycles/byte on optimized unrolled path */
}

/* ========================================================================
 * Benchmark 3: memset fill throughput
 * ======================================================================== */

void bench_memset_throughput(void) {
    bench_timer_t timer;
    char buffer[512];
    int iterations = 50;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        memset(buffer, 0xAA, 512);
    }
    bench_end(&timer);

    /* Result: cycles per byte filled
     * Expected: 1-2 cycles/byte */
}

/* ========================================================================
 * Benchmark 4: strlen on various lengths
 * ======================================================================== */

void bench_strlen_short(void) {
    bench_timer_t timer;
    char str[32] = "hello";
    int iterations = 1000;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        strlen(str);
    }
    bench_end(&timer);

    /* Result: cycles for strlen("hello")
     * Expected: 30-50 cycles */
}

void bench_strlen_medium(void) {
    bench_timer_t timer;
    char str[256] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    int iterations = 100;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        strlen(str);
    }
    bench_end(&timer);

    /* Result: cycles for strlen(long_string)
     * Expected: 1-2 cycles/byte + overhead */
}

/* ========================================================================
 * Benchmark 5: strcmp on various patterns
 * ======================================================================== */

void bench_strcmp_equal(void) {
    bench_timer_t timer;
    char s1[64] = "benchmark";
    char s2[64] = "benchmark";
    int iterations = 1000;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        strcmp(s1, s2);
    }
    bench_end(&timer);

    /* Result: cycles for strcmp on equal strings
     * Expected: 50-80 cycles */
}

void bench_strcmp_different(void) {
    bench_timer_t timer;
    char s1[64] = "benchmark";
    char s2[64] = "different";
    int iterations = 1000;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        strcmp(s1, s2);
    }
    bench_end(&timer);

    /* Result: cycles for strcmp on different strings
     * Expected: 50-100 cycles (depends on first diff position) */
}

/* ========================================================================
 * Benchmark 6: qsort on various array sizes
 * ======================================================================== */

static int cmp_int(const void *a, const void *b) {
    int av = *(int *)a;
    int bv = *(int *)b;
    return av - bv;
}

void bench_qsort_small(void) {
    bench_timer_t timer;
    int arr[8] = {7, 3, 8, 1, 5, 2, 6, 4};
    int iterations = 100;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        int copy[8] = {7, 3, 8, 1, 5, 2, 6, 4};
        qsort(copy, 8, sizeof(int), cmp_int);
    }
    bench_end(&timer);

    /* Result: cycles per qsort on 8 elements
     * Expected: 200-300 cycles (uses insertion sort internally) */
}

void bench_qsort_medium(void) {
    bench_timer_t timer;
    int arr[32];
    int i, j;

    /* Initialize with reverse-sorted data (worst case for naive sort) */
    for (i = 0; i < 32; i++) {
        arr[i] = 32 - i;
    }

    bench_start(&timer);
    for (j = 0; j < 10; j++) {
        int copy[32];
        for (i = 0; i < 32; i++) {
            copy[i] = arr[i];
        }
        qsort(copy, 32, sizeof(int), cmp_int);
    }
    bench_end(&timer);

    /* Result: cycles per qsort on 32 elements (worst case)
     * Expected: 2000-3000 cycles */
}

/* ========================================================================
 * Benchmark 7: bsearch on sorted array
 * ======================================================================== */

void bench_bsearch_hit(void) {
    bench_timer_t timer;
    int arr[64];
    int i;

    /* Initialize with sorted data */
    for (i = 0; i < 64; i++) {
        arr[i] = i * 2;
    }

    int needle = 64;
    int iterations = 1000;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        bsearch(&needle, arr, 64, sizeof(int), cmp_int);
    }
    bench_end(&timer);

    /* Result: cycles for bsearch on 64-element array (hit)
     * Expected: 150-250 cycles (log₂(64) ≈ 6 iterations) */
}

void bench_bsearch_miss(void) {
    bench_timer_t timer;
    int arr[64];
    int i;

    /* Initialize with sorted data */
    for (i = 0; i < 64; i++) {
        arr[i] = i * 2;
    }

    int needle = 999;  /* Not in array */
    int iterations = 1000;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        bsearch(&needle, arr, 64, sizeof(int), cmp_int);
    }
    bench_end(&timer);

    /* Result: cycles for bsearch on 64-element array (miss)
     * Expected: 150-250 cycles (always log₂(64) iterations) */
}

/* ========================================================================
 * Benchmark 8: rand() generation
 * ======================================================================== */

void bench_rand_generation(void) {
    bench_timer_t timer;
    int iterations = 10000;
    int i;

    bench_start(&timer);
    for (i = 0; i < iterations; i++) {
        int x = rand();
        (void)x;  /* Prevent optimization */
    }
    bench_end(&timer);

    /* Result: cycles per rand() call
     * Expected: 50-100 cycles (includes MEGA65 RNG polling) */
}

/* ========================================================================
 * Main benchmark runner
 * ======================================================================== */

int main(void) {
    /* Run all benchmarks */
    bench_malloc_rapid();
    bench_memcpy_throughput();
    bench_memset_throughput();
    bench_strlen_short();
    bench_strlen_medium();
    bench_strcmp_equal();
    bench_strcmp_different();
    bench_qsort_small();
    bench_qsort_medium();
    bench_bsearch_hit();
    bench_bsearch_miss();
    bench_rand_generation();

    return 0;  /* Success */
}
