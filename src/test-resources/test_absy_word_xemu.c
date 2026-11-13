/* test_absy_word_xemu.c — 16-bit elements of small static arrays read and
 * written as name,y / name+1,y, in the block of the element's address and
 * in others (#568), and the cases that must keep a pointer
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's function: ++pos[i] with a wrap, the element added to a sum
 *   [1] the same with a loop variable as the index, several arrays
 *   [2] an element's address kept while the index changes (in its block,
 *       in another block, in a loop): the first element is the one written
 *   [3] word stores of constants, variables and pointers; a struct array's
 *       word member
 *   [4] an array of more than 256 bytes (not this path): the same results
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

signed int pos[3];
unsigned int sum;
signed int cx[3], cy[3];
unsigned char every[3] = {1, 2, 3};
unsigned int tab[8];
unsigned char *ptrs[4];
unsigned char bytes[4];
struct rec { unsigned char k; unsigned int w; unsigned char z; } recs[5];
unsigned int big[200];

__attribute__((noinline)) void step(unsigned char i) {
    if (++pos[i] >= 80) pos[i] = -3;
    sum += pos[i];
}

__attribute__((noinline)) void tick(unsigned char t) {
    unsigned char i;
    for (i = 0; i < 3; i++) {
        if (!(t % every[i]) && ++cx[i] >= 4) cx[i] = -3;
        if (cx[i] < 0) cy[i] -= 2; else cy[i] += cx[i];
    }
}

__attribute__((noinline)) void keep(unsigned char i, unsigned char c) {
    unsigned int *p = &tab[i];
    i++;
    *p = 0x1111;                  /* tab[i - 1] */
    p = &tab[i];
    if (c) i += 2;
    *p += 0x0101;                 /* tab[the i before the if] */
    tab[i] = 0x2222;
}

__attribute__((noinline)) void fill(unsigned char n, unsigned int v) {
    unsigned char i = 0;
    while (i < n) {
        unsigned int *p = &tab[i];
        i++;
        if (v & 1) *p = v; else *p = i;
        v += 0x101;
    }
}

__attribute__((noinline)) void words(unsigned char i, unsigned int v) {
    tab[i] = 0xFFFD;
    tab[i + 1] = v;
    ptrs[i] = &bytes[i];
    recs[i].w = v + 1;
    recs[i].k = (unsigned char)v;
    if (v > 0x1000) recs[i].w += 0x10; else recs[i].w = 0;
    recs[i].z = 0x7E;
}

__attribute__((noinline)) unsigned int bigsum(unsigned char i) {
    if (++big[i] >= 3) big[i] = 0x4000;
    big[i + 1] = big[i] + 1;
    return big[i] + big[i + 1];
}

int main(void) {
    unsigned char k, ok;

    pos[0] = 78; pos[1] = -1; pos[2] = 5;
    step(0);                      /* 79 */
    step(0);                      /* 80 -> -3 */
    step(1);                      /* 0 */
    step(2);                      /* 6 */
    OUT[0] = pos[0] == -3 && pos[1] == 0 && pos[2] == 6 && sum == (unsigned int)(79 - 3 + 0 + 6);

    for (k = 1; k <= 6; k++) tick(k);
    /* cx[0]: +1 each tick, wraps to -3 at 4: 1 2 3 -3 -2 -1; cx[1]: ticks 2 4 6: 1 2 3; cx[2]: ticks 3 6: 1 2 */
    ok = cx[0] == -1 && cx[1] == 3 && cx[2] == 2;
    /* cy[0]: 1 3 6 4 2 0; cy[1]: 0 1 2 4 6 9; cy[2]: 0 0 1 2 3 5 */
    OUT[1] = ok && cy[0] == 0 && cy[1] == 9 && cy[2] == 5;

    keep(1, 0);                   /* tab[1] = $1111, tab[2] = $0101 then $2222 */
    ok = tab[1] == 0x1111 && tab[2] == 0x2222 && tab[3] == 0;
    tab[2] = 0;
    keep(1, 1);                   /* tab[2] = $0101, tab[4] = $2222 */
    ok = ok && tab[1] == 0x1111 && tab[2] == 0x0101 && tab[4] == 0x2222;
    fill(4, 0x0701);              /* $0701, 2, $0903, 4 */
    ok = ok && tab[0] == 0x0701 && tab[1] == 2 && tab[2] == 0x0903 && tab[3] == 4 && tab[4] == 0x2222;
    OUT[2] = ok;

    words(2, 0x1234);
    ok = tab[2] == 0xFFFD && tab[3] == 0x1234 && ptrs[2] == &bytes[2] && ptrs[1] == 0 &&
         recs[2].w == 0x1245 && recs[2].k == 0x34 && recs[2].z == 0x7E && recs[1].z == 0 && recs[3].k == 0;
    words(4, 0x0800);
    OUT[3] = ok && recs[4].w == 0 && recs[4].k == 0 && recs[4].z == 0x7E && tab[4] == 0xFFFD && tab[5] == 0x0800;

    ok = bigsum(150) == 1 + 2 && big[150] == 1 && big[151] == 2;
    bigsum(150);                  /* 2 */
    ok = ok && bigsum(150) == 0x4000 + 0x4001 && big[150] == 0x4000 && big[149] == 0;
    OUT[4] = ok;

    OUT[5] = 0xAA;
    return 0;
}
