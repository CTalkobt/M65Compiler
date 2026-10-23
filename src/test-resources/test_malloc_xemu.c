/* test_malloc_xemu.c — malloc / free / calloc / realloc on the default heap
 *
 * Results at $C0C0 (01 = pass):
 *   [0] malloc returns a pointer inside the heap (above the program), usable
 *   [1] blocks don't overlap; odd sizes get enough room
 *   [2] free + malloc reuses memory; freeing neighbours coalesces them
 *   [3] calloc zeroes the block
 *   [4] realloc keeps the contents (grow and shrink), NULL/0 forms
 *   [5] a large allocation succeeds and a too-large one returns NULL; sizes
 *       near 64 KB and calloc products over 64 KB are NULL, not short
 *       blocks (#203), and stale bytes in the multiplier's upper inputs
 *       don't make a small calloc fail
 *   [6] 01
 *   [7] $AA sentinel
 */
#include <stdlib.h>
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char marker[4];   /* in BSS: the heap starts after it */

static int filled(unsigned char *p, int n, unsigned char v) {
    int i;
    for (i = 0; i < n; i++)
        if (p[i] != v) return 0;
    return 1;
}

int main(void) {
    unsigned char *a, *b, *c, *d, *big;
    unsigned int ua;
    int ok;   /* $C0C0 reads as ROM: build results in a variable, then store */

    a = malloc(10);
    ua = (unsigned int)a;
    memset(a, 0x11, 10);
    OUT[0] = a != 0 && ua > (unsigned int)marker && ua < 0xB000u && filled(a, 10, 0x11);

    b = malloc(3);
    c = malloc(7);
    memset(b, 0x22, 3);
    memset(c, 0x33, 7);
    OUT[1] = b && c && b != a && c != b && filled(a, 10, 0x11) && filled(b, 3, 0x22) && filled(c, 7, 0x33);

    /* free b and c (neighbours), then a block bigger than either fits there */
    free(b);
    free(c);
    d = malloc(10);
    OUT[2] = d == b && filled(a, 10, 0x11);

    free(d);
    d = calloc(5, 4);
    OUT[3] = d && filled(d, 20, 0);

    memset(d, 0x44, 20);
    d = realloc(d, 200);
    ok = d && filled(d, 20, 0x44);
    d = realloc(d, 8);
    ok = ok && d && filled(d, 8, 0x44) && realloc(0, 0) == 0;
    b = realloc(0, 6);
    OUT[4] = ok && b != 0 && realloc(b, 0) == 0;

    big = malloc(20000u);
    ok = big != 0 && malloc(60000u) == 0;
    if (big) { big[0] = 1; big[19999u] = 2; ok = ok && big[0] == 1 && big[19999u] == 2; }
    ok = ok && malloc(0xFFFFu) == 0 && malloc(0xFFFDu) == 0;
    ok = ok && calloc(300, 300) == 0 && calloc(0x1000u, 0x11) == 0;
    ok = ok && big && realloc(big, 0xFFFEu) == 0 && big[19999u] == 2;
    *(volatile unsigned char *)0xD773 = 0xFF;   /* the multiplier's upper input bytes */
    *(volatile unsigned char *)0xD777 = 0xFF;
    d = calloc(3, 2);
    ok = ok && d && filled(d, 6, 0);
    OUT[5] = ok;

    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
