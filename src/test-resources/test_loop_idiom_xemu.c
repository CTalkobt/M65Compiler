/* test_loop_idiom_xemu.c — copy and fill loops as memmove / memset (#323,
 * cc45 -floop-idioms; test_xemu.sh builds it so): the results and the loop
 * variables' final values must be the loops'
 *
 * (Only loops worth a call become one: two arrays or more, a row loop, or
 * elements of 3 bytes or more; the loops here are such ones.)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a forward copy of two arrays; a shift down (a[i] = a[i + 1]) of longs
 *   [1] the high-score insert: a backward shift of a struct's member arrays,
 *       2-D rows with an inner loop (name[5][3]), longs
 *   [2] fills: 0, -1 into ints, a byte value; a <= bound
 *   [3] a loop that runs no time (arrays untouched, i unchanged); final i
 *   [4] loops that must stay loops (a smear a[i] = a[i - 1] going up, a
 *       body with a call): the loops' results
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))
#define HS 5

static struct {
    unsigned char version;
    unsigned long score[HS];
    unsigned char name[HS][3];
    unsigned char round[HS];
} hs;
unsigned char src[20], dst[20], src2[20], dst2[20];
unsigned long lw[12];
int words[12];
unsigned char bytes[30];
unsigned char smear[10];
unsigned char counter;

N static unsigned char twice(unsigned char x) { counter++; return (unsigned char)(x * 2); }

N static unsigned char copy_test(unsigned char n) {
    unsigned char i;
    for (i = 0; i < 20; i++) { src[i] = i + 1; src2[i] = 100 + i; }
    for (i = 0; i < 20; i++) { dst[i] = src[i]; dst2[i] = src2[i]; }
    if (i != 20 || dst[0] != 1 || dst[19] != 20 || dst2[7] != 107) return 0;
    for (i = 0; i < 12; i++) lw[i] = 100000UL + i;
    for (i = 2; i < n; i++) lw[i] = lw[i + 1];         /* shift down: ... 100003 ... 100011 100011 */
    return lw[1] == 100001UL && lw[2] == 100003UL && lw[10] == 100011UL && lw[11] == 100011UL && i == n;
}

N static unsigned char insert_test(unsigned char rank) {
    unsigned char i, k;
    for (i = 0; i < HS; i++) {
        hs.score[i] = 1000UL * (i + 1);
        hs.name[i][0] = 'a' + i; hs.name[i][1] = 'b' + i; hs.name[i][2] = 'c' + i;
        hs.round[i] = 10 + i;
    }
    for (i = HS - 1; i > rank; i--) {
        hs.score[i] = hs.score[i - 1];
        for (k = 0; k < 3; k++) hs.name[i][k] = hs.name[i - 1][k];
        hs.round[i] = hs.round[i - 1];
    }
    return i == rank && k == 3 && hs.score[0] == 1000 && hs.score[1] == 2000 && hs.score[2] == 2000 &&
           hs.score[3] == 3000 && hs.score[4] == 4000 && hs.name[2][0] == 'b' && hs.name[4][2] == 'f' &&
           hs.name[1][1] == 'c' && hs.round[2] == 11 && hs.round[4] == 13 && hs.version == 0;
}

N static unsigned char fill_test(unsigned char n) {
    unsigned char i;
    for (i = 0; i < 12; i++) { words[i] = 0x1234; lw[i] = 7; }
    for (i = 1; i < 11; i++) { words[i] = 0; lw[i] = 0; }
    for (i = 0; i <= 5; i++) { words[i] = -1; lw[i] = -1; }
    for (i = 3; i < n; i++) { bytes[i] = 0x5A; dst2[i - 3] = 0xA5; }
    return words[0] == -1 && words[5] == -1 && words[6] == 0 && words[10] == 0 && words[11] == 0x1234 &&
           lw[5] == 0xFFFFFFFFUL && lw[6] == 0 && lw[11] == 7 &&
           bytes[2] == 0 && bytes[3] == 0x5A && bytes[n - 1] == 0x5A && bytes[n] == 0 &&
           dst2[0] == 0xA5 && dst2[n - 4] == 0xA5 && i == n;
}

N static unsigned char none_test(unsigned char from) {
    unsigned char i, before = dst[5], before2 = dst2[5];
    for (i = from; i < 10; i++) { dst[i] = 0; dst2[i] = 0; }   /* from >= 10: never runs */
    return i == from && dst[5] == before && dst2[5] == before2;
}

N static unsigned char loop_test(void) {
    unsigned char i;
    for (i = 0; i < 10; i++) smear[i] = i + 1;
    for (i = 1; i < 10; i++) smear[i] = smear[i - 1];   /* a smear: every one 1 */
    counter = 0;
    for (i = 0; i < 5; i++) bytes[i] = twice(i);
    return smear[9] == 1 && smear[5] == 1 && counter == 5 && bytes[4] == 8;
}

int main(void) {
    OUT[0] = copy_test(11);
    OUT[1] = insert_test(1);
    OUT[2] = fill_test(25);
    OUT[3] = none_test(12) && dst[19] == 20;
    OUT[4] = loop_test();
    OUT[5] = 0xAA;
    return 0;
}
