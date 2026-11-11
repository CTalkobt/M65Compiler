/* test_compound_once_xemu.c — a compound assignment evaluates its target
 * once (C11 6.5.16.2p3, #526)
 *
 * a[i++] |= v and *p++ += v: the index or pointer stepped once, the old value
 * read from and the result written to the same element. Also a pre-increment,
 * a nested assignment and a call in the subscript, a 16-bit and a float
 * element, and the loop that found it (while (w--) clr[o++] |= 0x20).
 * Each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 04 05 06 07 08 AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

unsigned char a[2000];
unsigned int w16[8];
float fl[4];
unsigned char calls;

static unsigned int next(unsigned int k) { calls++; return k; }

static void f(unsigned int i) { a[i++] |= 0x20; a[i] |= 0x01; }
static void g(unsigned char *p) { *p++ |= 0x20; *p += 3; }

int main(void) {
    unsigned int i, j, o, w;
    unsigned char *p;
    for (i = 0; i < 9; i++) RESULT(i) = 0;
    for (i = 0; i < 2000; i++) a[i] = 0;

    /* 1: a[i++] |= v: one step, the same element */
    i = 5;
    a[5] = 0x40; a[6] = 0;
    a[i++] |= 0x20;
    if (i == 6 && a[5] == 0x60 && a[6] == 0) RESULT(0) = 0x01;

    /* 2: the issue's f(): a[i] then a[i+1] */
    a[100] = 0; a[101] = 0; a[102] = 0;
    f(100);
    if (a[100] == 0x20 && a[101] == 0x01 && a[102] == 0) RESULT(1) = 0x02;

    /* 3: *p++ op= v, then the next one */
    a[200] = 1; a[201] = 1; a[202] = 9;
    g(&a[200]);
    p = &a[300]; a[300] = 10; a[301] = 0;
    *p++ += 5;
    if (a[200] == 0x21 && a[201] == 4 && a[202] == 9 && p == &a[301] && a[300] == 15 && a[301] == 0)
        RESULT(2) = 0x03;

    /* 4: a pre-increment and other operators */
    i = 9; a[10] = 0xFF; a[11] = 0xFF;
    a[++i] &= 0x0F;
    j = 20; a[20] = 7;
    a[j--] -= 2;
    if (i == 10 && a[10] == 0x0F && a[11] == 0xFF && j == 19 && a[20] == 5 && a[19] == 0) RESULT(3) = 0x04;

    /* 5: a call and an assignment in the subscript: run once */
    calls = 0; a[400] = 1;
    a[next(400)] += 1;
    j = 0; a[401] = 3;
    a[j = 401] <<= 2;
    if (calls == 1 && a[400] == 2 && j == 401 && a[401] == 12) RESULT(4) = 0x05;

    /* 6: a 16-bit element */
    i = 2; w16[2] = 0x1234; w16[3] = 0;
    w16[i++] ^= 0x00FF;
    if (i == 3 && w16[2] == 0x12CB && w16[3] == 0) RESULT(5) = 0x06;

    /* 7: a float element */
    i = 1; fl[1] = 2.5; fl[2] = 100.0;
    fl[i++] *= 4.0;
    if (i == 2 && fl[1] == 10.0 && fl[2] == 100.0) RESULT(6) = 0x07;

    /* 8: the loop that found it: every cell once */
    for (i = 500; i < 520; i++) a[i] = 1;
    o = 505; w = 10;
    while (w--) a[o++] |= 0x20;
    for (i = 500, j = 1; i < 520; i++)
        if (a[i] != ((i >= 505 && i < 515) ? 0x21 : 1)) j = 0;
    if (j && o == 515) RESULT(7) = 0x08;

    RESULT(8) = 0xAA;
    return 0;
}
