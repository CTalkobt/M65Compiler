/* test_comma_expr_xemu.c — the comma operator where the grammar takes a full
 * expression (#572: these took only an assignment expression, and the comma
 * was a syntax error)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] if, while and do … while conditions
 *   [1] a switch's controlling expression
 *   [2] return
 *   [3] a subscript, the middle operand of ?:, sizeof
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

volatile unsigned char src = 7;
unsigned char u, k, n;
unsigned char arr[4] = { 10, 20, 30, 40 };

unsigned char ret_comma(unsigned char x) {
    return u = x, x + 1;
}

unsigned char sw(unsigned char x) {
    switch ((u = x), x & 3) {
    case 1: return 11;
    case 2: return 22;
    }
    return 0;
}

int main(void) {
    unsigned char ok, r;

    /* [0] */
    ok = 1;
    u = 0; k = 9; r = 0;
    if ((u = src), k == 9) r = 1;
    if (r != 1 || u != 7) ok = 0;
    r = 0;
    if (u = 3, k == 8) r = 1; else r = 2;
    if (r != 2 || u != 3) ok = 0;
    n = 0; k = 0;
    while (n++, k < 3) k++;
    if (n != 4 || k != 3) ok = 0;
    n = 0; k = 0;
    do { k++; } while (n += 2, k < 3);
    if (n != 6 || k != 3) ok = 0;
    OUT[0] = ok;

    /* [1] */
    ok = 1;
    if (sw(5) != 11 || u != 5) ok = 0;
    if (sw(6) != 22 || u != 6) ok = 0;
    if (sw(4) != 0 || u != 4) ok = 0;
    OUT[1] = ok;

    /* [2] */
    ok = 1;
    if (ret_comma(41) != 42 || u != 41) ok = 0;
    OUT[2] = ok;

    /* [3] */
    ok = 1;
    u = 0;
    r = arr[u = src, 2];
    if (r != 30 || u != 7) ok = 0;
    k = 1;
    r = k ? u = 9, 5 : 6;
    if (r != 5 || u != 9) ok = 0;
    k = 0;
    r = k ? u = 1, 5 : 6;
    if (r != 6 || u != 9) ok = 0;
    if (sizeof(u, arr[0]) != 1 || sizeof(k, 1L) != 4) ok = 0;
    OUT[3] = ok;

    OUT[4] = 0xAA;
    return 0;
}
