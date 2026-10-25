/* test_lsr_xemu.c — loop strength reduction: g[i] with an element size not
 * a power of 2, and i * K, in a loop over i, as a variable set before the
 * loop and stepped with i (the expected values computed outside cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a 2D array's rows (80 bytes) over an int counter, an inner loop
 *       over the columns: filled, then summed
 *   [1] an unsigned char counter (y < 25) times 80 and times 400
 *   [2] a count down by 2 from a parameter: 3-byte structs, and i * 7
 *   [3] an unsigned char counter up to 254 (y < 255): the sum of y * 3
 *   [4] the product used before and after the counter's update in one
 *       iteration (a while loop), and the counter's value after the loop
 *   [5] 01
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char grid[12][80];
/* (static before a tagged definition with its variable: parsed since this test) */
static struct trip { unsigned char a, b, c; } tr[20];
static volatile unsigned char vstart = 19;

static int test_rows(void) {
    int r, c;
    unsigned long sum = 0;
    for (r = 0; r < 12; r++)
        for (c = 0; c < 80; c++) grid[r][c] = (unsigned char)(r * 3 + c);
    for (r = 0; r < 12; r++)
        for (c = 0; c < 80; c += 13) sum += grid[r][c];
    /* sum over r < 12, c in {0, 13, ..., 78}: 7 columns of r * 3 + c */
    return sum == 4662UL && grid[11][79] == 112 && grid[5][0] == 15;
}

static int test_byte_rows(void) {
    unsigned char y;
    unsigned int a = 0, b = 0;
    unsigned long s = 0;
    for (y = 0; y < 25; y++) {
        a = y * 80u;
        b = y * 400u;
        s += a + (b & 0xFF);
    }
    /* a: 80 * 300; b & $FF: (400 y) & 255 summed, from Python */
    return a == 1920u && b == 9600u && s == 27008UL;
}

static int count_down(signed int start) {
    signed int i;
    unsigned int prod = 0;
    for (i = start; i > 0; i -= 2) {
        tr[i].a = (unsigned char)i;
        tr[i].b = (unsigned char)(i * 2);
        tr[i].c = (unsigned char)(i * 7);
        prod += (unsigned int)i * 7u;
    }
    return (int)prod;   /* 7 * (19 + 17 + ... + 1) = 700 */
}

static int test_count_down(void) {
    int k;
    if (count_down(vstart) != 700) return 0;
    for (k = 1; k < 20; k += 2)
        if (tr[k].a != k || tr[k].b != k * 2 || tr[k].c != k * 7) return 0;
    return tr[2].a == 0 && tr[0].c == 0;
}

static int test_byte_255(void) {
    unsigned char y;
    unsigned long s = 0;
    for (y = 0; y < 255; y++) s += (unsigned int)(y * 3u);
    return s == 97155UL && y == 255;   /* 3 * (0 + ... + 254) */
}

static int test_while(void) {
    signed int i = 0;
    unsigned int before = 0, after = 0;
    while (i < 10) {
        before += i * 9u;
        i++;
        after += i * 9u;
    }
    /* before: 9 * (0..9) = 405; after: 9 * (1..10) = 495 */
    return before == 405u && after == 495u && i == 10;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 8; i++) OUT[i] = 0;
    OUT[0] = test_rows() ? 1 : 0;
    OUT[1] = test_byte_rows() ? 1 : 0;
    OUT[2] = test_count_down() ? 1 : 0;
    OUT[3] = test_byte_255() ? 1 : 0;
    OUT[4] = test_while() ? 1 : 0;
    OUT[5] = 1;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
