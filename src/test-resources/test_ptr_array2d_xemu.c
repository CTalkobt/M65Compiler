/* test_ptr_array2d_xemu.c — subscripts through a pointer to a 2-D array:
 * (*p)[i][j] of a T (*p)[N][M] (tiny-AES's state_t *; #129: the second
 * subscript stepped by 2 bytes for a char array)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] (*s)[i][j] = v over the whole char array: each byte in its place
 *   [1] constant subscripts: (*s)[2][3], (*s)[1][0]
 *   [2] read and ^= through it (AES's AddRoundKey)
 *   [3] an int array: (*p)[1][1]
 *   [4] a 3-D array: (*q)[1][2][3]
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned char st[4][4];

static void fill(st *s) {
    unsigned char i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            (*s)[i][j] = i * 16 + j;
}
static void consts(st *s) { (*s)[2][3] = 0x77; (*s)[1][0] = 0x66; }
static void xorKey(st *s, const unsigned char *k) {
    unsigned char i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            (*s)[i][j] ^= k[i * 4 + j];
}

int main(void) {
    unsigned char buf[16], key[16], k, ok;
    int iv[3][2];
    unsigned char c3[2][3][4];

    fill((st *)buf);
    ok = 1;
    for (k = 0; k < 16; k++) if (buf[k] != (k >> 2) * 16 + (k & 3)) ok = 0;
    OUT[0] = ok;

    for (k = 0; k < 16; k++) buf[k] = 0xEE;
    consts((st *)buf);
    ok = buf[11] == 0x77 && buf[4] == 0x66;
    for (k = 0; k < 16; k++) if (k != 11 && k != 4 && buf[k] != 0xEE) ok = 0;
    OUT[1] = ok;

    for (k = 0; k < 16; k++) { buf[k] = k * 17 + 3; key[k] = k * 7 + 1; }
    xorKey((st *)buf, key);
    ok = 1;
    for (k = 0; k < 16; k++) if (buf[k] != (unsigned char)((k * 17 + 3) ^ (k * 7 + 1))) ok = 0;
    OUT[2] = ok;

    {
        int (*p)[3][2] = &iv;
        for (k = 0; k < 6; k++) ((int *)iv)[k] = 0;
        (*p)[1][1] = 0x1234;
        OUT[3] = ((int *)iv)[3] == 0x1234 && ((int *)iv)[2] == 0 && ((int *)iv)[4] == 0;
    }
    {
        unsigned char (*q)[2][3][4] = &c3;
        for (k = 0; k < 24; k++) ((unsigned char *)c3)[k] = 0;
        (*q)[1][2][3] = 0x5A;
        OUT[4] = ((unsigned char *)c3)[12 + 8 + 3] == 0x5A && ((unsigned char *)c3)[22] == 0;
    }
    OUT[5] = 0xAA;
    return 0;
}
