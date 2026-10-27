/* bank_rows.c — arrays of arrays through far pointers (#347): a __bank(n)
 * T t[R][C] (and three dimensions), far pointers to arrays (T (__far *p)[N],
 * a typedef'd row: pair __far *p, casts to them) and the same for normal
 * pointers (pair *p, T (*p)[N] parameters).
 *
 * Results at $C0C0 (01 = pass):
 *   +0 a banked [3][2] table read; a [2][3][4] one written and read
 *   +1 sizeof of a row and of an element, p + 1 moves by a row
 *   +2 T (__far *p)[N] and pair __far *p parameters, written through
 *   +3 far locals and casts: pair __far *p = (pair __far *)..., a cast indexed
 *   +4 normal pointers: pair *p and T (*p)[N] parameters, a pair *q local
 *   +5 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned int pair[2];

__bank(4) const unsigned int t[3][2] = {{1, 2}, {3, 4}, {5, 6}};
__bank(5) unsigned char c3[2][3][4];
unsigned int near_rows[3][2] = {{10, 20}, {30, 40}, {50, 60}};

static unsigned int get_t(unsigned char k, unsigned char v) { return t[k][v]; }

static unsigned int far_param(const unsigned int (__far *p)[2], unsigned char k, unsigned char v) { return p[k][v]; }
static unsigned int far_pair(const pair __far *p, unsigned char k, unsigned char v) { return p[k][v]; }
static void far_set(pair __far *p, unsigned char k, unsigned char v, unsigned int x) { p[k][v] = x; }

static unsigned int near_pair(pair *p, unsigned char k, unsigned char v) { return p[k][v]; }
static unsigned int near_param(unsigned int (*p)[2], unsigned char k, unsigned char v) { return p[k][v]; }

int main(void) {
    unsigned char i, j, k;
    unsigned int s = 0;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 2; j++) s += get_t(i, j) * (i + 1);
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
            for (k = 0; k < 4; k++) c3[i][j][k] = (unsigned char)(i * 100 + j * 10 + k);
    c3[1][2][3] += 5;
    OUT[0] = s == 1 + 2 + 2 * (3 + 4) + 3 * (5 + 6) && c3[0][0][0] == 0 && c3[1][0][2] == 102 &&
             c3[0][2][1] == 21 && c3[1][2][3] == 128;

    OUT[1] = sizeof(t[1]) == 4 && sizeof(c3[1]) == 12 && sizeof(c3[1][2]) == 4 && sizeof(t[0][0]) == 2 &&
             (unsigned long)(&t[1]) - (unsigned long)(&t[0]) == 4 &&
             (unsigned long)((const unsigned int (__far *)[2])&t[0] + 1) == (unsigned long)&t[1];

    far_set((pair __far *)&c3[0][0][0], 1, 1, 0x1234);   /* c3[0][1][2..3] */
    OUT[2] = far_param((const unsigned int (__far *)[2])&t[0], 2, 1) == 6 &&
             far_pair((const pair __far *)&t[0], 1, 0) == 3 && c3[0][1][2] == 0x34 && c3[0][1][3] == 0x12;

    {
        const pair __far *p = (const pair __far *)&t[0];
        const unsigned int (__far *q)[2] = (const unsigned int (__far *)[2])&t[1];
        OUT[3] = p[2][0] == 5 && q[1][1] == 6 && q[0][0] == 3 &&
                 ((const pair __far *)&t[0])[1][1] == 4 &&
                 ((const unsigned int (__far *)[2])&t[0])[2][1] == 6;
    }

    {
        pair *q = near_rows;
        OUT[4] = near_pair(near_rows, 2, 1) == 60 && near_param(near_rows, 1, 0) == 30 && q[1][1] == 40;
    }

    OUT[5] = 0xAA;
    return 0;
}
