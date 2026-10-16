/* test_qsort_xemu.c — qsort (not recursive: the larger part of each
 * partition waits on a stack; insertion sort for 8 elements or fewer) and
 * bsearch
 *
 * Results at $C0C0 (01 = pass):
 *   [0] ints: 0, 1, 2, 8, 9 elements (the insertion-sort cutoff)
 *   [1] 100 and 300 ints in pseudo-random order: sorted, same sum
 *   [2] already sorted, reversed, all equal, two values
 *   [3] chars (size 1) and a 3-byte struct sorted by one member
 *   [4] 5-byte elements (a float-sized record) by a 16-bit key
 *   [5] bsearch: found first, middle and last; not found below, between
 *       and above; no elements; one; 5-byte records
 *   [6] $AA sentinel
 */
#include <stdlib.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static signed int cmp_int(const void *a, const void *b) {
    signed int x = *(const signed int *)a, y = *(const signed int *)b;
    return x < y ? -1 : x > y ? 1 : 0;
}
static signed int cmp_char(const void *a, const void *b) {
    return (signed int)*(const unsigned char *)a - (signed int)*(const unsigned char *)b;
}
struct S3 { unsigned char tag; unsigned int key; };
static signed int cmp_s3(const void *a, const void *b) {
    unsigned int x = ((const struct S3 *)a)->key, y = ((const struct S3 *)b)->key;
    return x < y ? -1 : x > y ? 1 : 0;
}
struct S5 { unsigned int key; unsigned char pad[3]; };
static signed int cmp_s5(const void *a, const void *b) {
    unsigned int x = ((const struct S5 *)a)->key, y = ((const struct S5 *)b)->key;
    return x < y ? -1 : x > y ? 1 : 0;
}

static signed int v[300];
static unsigned int seed = 12345;
static signed int rnd(void) { seed = seed * 25173u + 13849u; return (signed int)(seed >> 3) - 4000; }

static unsigned char sorted(signed int *a, unsigned int n) {
    unsigned int i;
    for (i = 1; i < n; i++) if (a[i - 1] > a[i]) return 0;
    return 1;
}
static long sum(signed int *a, unsigned int n) { long s = 0; unsigned int i; for (i = 0; i < n; i++) s += a[i]; return s; }

static unsigned char trial(unsigned int n) {
    unsigned int i; long s;
    for (i = 0; i < n; i++) v[i] = rnd();
    s = sum(v, n);
    qsort(v, n, sizeof v[0], cmp_int);
    return sorted(v, n) && sum(v, n) == s;
}

int main(void) {
    unsigned int i;
    unsigned char ok;
    static unsigned char cs[10] = {9, 3, 200, 0, 3, 77, 1, 255, 42, 5};
    static struct S3 s3[6] = {{1, 500}, {2, 3}, {3, 70000u & 0xFFFF}, {4, 3}, {5, 1}, {6, 499}};
    static struct S5 s5[7];

    OUT[0] = trial(0) && trial(1) && trial(2) && trial(8) && trial(9);
    OUT[1] = trial(100) && trial(300);

    for (i = 0; i < 50; i++) v[i] = (signed int)i;
    qsort(v, 50, 2, cmp_int); ok = sorted(v, 50) && v[0] == 0 && v[49] == 49;
    for (i = 0; i < 50; i++) v[i] = (signed int)(50 - i);
    qsort(v, 50, 2, cmp_int); ok = ok && sorted(v, 50) && v[0] == 1 && v[49] == 50;
    for (i = 0; i < 40; i++) v[i] = 7;
    qsort(v, 40, 2, cmp_int); ok = ok && sorted(v, 40) && v[39] == 7;
    for (i = 0; i < 40; i++) v[i] = (i & 1) ? -1 : 1;
    qsort(v, 40, 2, cmp_int); ok = ok && sorted(v, 40) && v[19] == -1 && v[20] == 1;
    OUT[2] = ok;

    qsort(cs, 10, 1, cmp_char);
    ok = cs[0] == 0 && cs[1] == 1 && cs[2] == 3 && cs[3] == 3 && cs[9] == 255;
    qsort(s3, 6, sizeof s3[0], cmp_s3);
    ok = ok && sizeof s3[0] == 3 && s3[0].key == 1 && s3[0].tag == 5 && s3[3].key == 499 && s3[3].tag == 6 &&
         s3[5].key == (70000u & 0xFFFF) && s3[5].tag == 3 && (s3[1].tag == 2 || s3[1].tag == 4);
    OUT[3] = ok;

    for (i = 0; i < 7; i++) { s5[i].key = (unsigned int)(1000 - i * 111); s5[i].pad[0] = (unsigned char)i; }
    qsort(s5, 7, sizeof s5[0], cmp_s5);
    OUT[4] = sizeof s5[0] == 5 && s5[0].key == 1000 - 666 && s5[0].pad[0] == 6 && s5[6].key == 1000 && s5[6].pad[0] == 0;
    {
        static signed int key;
        static struct S5 k5;
        for (i = 0; i < 101; i++) v[i] = (signed int)(i * 2);       /* 0, 2 .. 200 */
        key = 0;   ok = bsearch(&key, v, 101, 2, cmp_int) == &v[0];
        key = 100; ok = ok && bsearch(&key, v, 101, 2, cmp_int) == &v[50];
        key = 200; ok = ok && bsearch(&key, v, 101, 2, cmp_int) == &v[100];
        key = -1;  ok = ok && bsearch(&key, v, 101, 2, cmp_int) == 0;
        key = 77;  ok = ok && bsearch(&key, v, 101, 2, cmp_int) == 0;
        key = 201; ok = ok && bsearch(&key, v, 101, 2, cmp_int) == 0;
        key = 0;   ok = ok && bsearch(&key, v, 0, 2, cmp_int) == 0 && bsearch(&key, v, 1, 2, cmp_int) == &v[0];
        k5.key = 1000 - 333;
        ok = ok && bsearch(&k5, s5, 7, sizeof s5[0], cmp_s5) == &s5[3];
        OUT[5] = ok;
    }
    OUT[6] = 0xAA;
    return 0;
}
