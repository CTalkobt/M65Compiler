/* test_elem_y_xemu.c — elements of global arrays as name+off,y instead of an
 * address built in a pointer (Issue #318); linked with test_elem_y_b.c, which
 * defines ext[] (extern here)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a byte table of 600 bytes at byte indices up to 255; t[i + 5]
 *   [1] an extern byte array: loads, stores, t[i]--, t[i] += 3, t[i] |= 0x80
 *   [2] the index changed between two accesses; in a loop (copy, fill, sum)
 *   [3] a value from a call stored; a small word array (stride 2)
 *   [4] a signed char index and a 2-D element (the pointer path, unchanged)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

extern unsigned char ext[40];
unsigned char big[600];
unsigned int words[20];
unsigned char grid[4][5];

N static unsigned char seven(void) { return 7; }

N static unsigned char big_test(unsigned char i) {
    big[i] = 200;
    big[i + 5] = 9;
    big[255] = 77;
    return big[i] == 200 && big[i + 5] == 9 && big[255] == 77 && big[(unsigned char)(i + 5)] == 9;
}

N static unsigned char ext_test(unsigned char i) {
    ext[i] = 10;
    ext[i]--;
    ext[i] += 3;
    ext[i + 1] = 1;
    ext[i + 1] |= 0x80;
    return ext[i] == 12 && ext[i + 1] == 0x81;
}

N static unsigned char index_test(unsigned char i) {
    unsigned char x, k, sum = 0;
    x = big[i];
    i++;
    big[i] = x + 1;                   /* the next element, not the same */
    for (k = 0; k < 10; k++) ext[k + 20] = big[k + 100];   /* copy */
    for (k = 0; k < 10; k++) big[k + 300 - 300] = 0;        /* fill (k + 0) */
    for (k = 20; k < 30; k++) sum += ext[k];
    return big[i] == x + 1 && sum == 10 * 5 && big[9] == 0;
}

N static unsigned char call_test(unsigned char i) {
    words[i] = 1000;
    words[i + 1] = words[i] + 234;
    big[i + 50] = seven();
    return words[i + 1] == 1234 && big[i + 50] == 7;
}

N static unsigned char other_test(signed char j, unsigned char r, unsigned char c) {
    big[j + 1] = 33;                  /* (a signed char index) */
    grid[r][c] = 44;
    return big[j + 1] == 33 && grid[r][c] == 44 && grid[2][3] == 44;
}

int main(void) {
    unsigned char k;
    for (k = 100; k < 110; k++) big[k] = 5;
    OUT[0] = big_test(200);
    OUT[1] = ext_test(4);
    OUT[2] = index_test(120);
    OUT[3] = call_test(3);
    OUT[4] = other_test(4, 2, 3) && big[5] == 33;
    OUT[5] = 0xAA;
    return 0;
}
