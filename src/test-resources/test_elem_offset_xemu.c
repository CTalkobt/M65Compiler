/* test_elem_offset_xemu.c — a[i + c] as element i of a shifted base
 * (foldElemIndexOffset): global arrays read and written at i + c, i - c,
 * with int and char elements, a char index, and an index changed between
 * the add and the access (not folded)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] neighbours: a[j] > a[j + 1] swaps (a bubble pass), int elements
 *   [1] a[i - 1] + a[i + 2] over the array, char elements
 *   [2] stores at b[i + 3] with a char index
 *   [3] x changed between x + c and the access
 *   [4] consecutive loads a[i], a[i + 1], a[i + 3] through one address (int
 *       and char), and i = n[i] (the load changes the index: recomputed)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static signed int a[10] = {5, 3, 9, 1, 7, 2, 8, 6, 4, 0};
static unsigned char c[12] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120};
static unsigned char b[16];
static signed int d[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static unsigned int n[8] = {3, 6, 1, 7, 2, 0, 5, 4};

int main(void) {
    unsigned int j, s;
    unsigned char i, k;
    signed int t, ok;

    for (j = 0; j < 9; j++)
        if (a[j] > a[j + 1]) { t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }
    OUT[0] = a[9] == 9 && a[0] == 3 && a[1] == 5 && a[8] == 0;

    s = 0;
    for (j = 1; j < 10; j++) s += c[j - 1] + c[j + 2];
    OUT[1] = s == (10 + 20 + 30 + 40 + 50 + 60 + 70 + 80 + 90) + (40 + 50 + 60 + 70 + 80 + 90 + 100 + 110 + 120);

    for (i = 0; i < 8; i++) b[i + 3] = (unsigned char)(i * 2);
    OUT[2] = b[3] == 0 && b[10] == 14 && b[2] == 0 && b[11] == 0;

    ok = 1;
    for (k = 0; k < 6; k++) {
        unsigned char m = k + 2;
        k++;                         /* k changes after m = k + 2 */
        if (d[m] != (signed int)(m + 1)) ok = 0;
        k--;
    }
    OUT[3] = ok;

    {
        unsigned int q = 0, r = 0, x;
        for (j = 0; j < 4; j++) q += (unsigned int)d[j] * 100 + (unsigned int)d[j + 1] * 10 + (unsigned int)d[j + 3];
        for (j = 0; j < 8; j++) r += c[j] + c[j + 1] + c[j + 3];
        x = 0;
        for (j = 0; j < 5; j++) { x = n[x]; s = n[x + 1]; }   /* x = n[x]: the index changes (0 3 7 4 2 1) */
        OUT[4] = q == 1162 && r == 1400 && x == 1 && s == n[2];
    }
    OUT[5] = 0xAA;
    return 0;
}
