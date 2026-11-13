/* test_loop_live_xemu.c — a value computed inside a loop and used after it
 * stays where it is over the whole loop (#561: n, read in the loop's test
 * and compared again after the loop, shared its register with a temporary
 * of the loop's body, which a break leaves by)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's function: the loop left by break, and by its test
 *   [1] the same with an int bound and counter
 *   [2] a nested loop: the inner loop left by break, the outer bound
 *       compared after both
 *   [3] a while loop left by break after work that needs temporaries
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char n, par[8], seen[2], found;
unsigned int wn, wpar[8], wfound;
unsigned char rows, cols, grid[4][4], at;

__attribute__((noinline)) void find(unsigned char p) {
    unsigned char k;
    for (k = 0; k < n; k++)
        if (par[k] == p && !(seen[k >> 3] & (1 << (k & 7)))) break;
    found = (k < n);       /* 1 = the loop ended with break */
}

__attribute__((noinline)) void wfind(unsigned int p) {
    unsigned int k;
    for (k = 0; k < wn; k++)
        if (wpar[k] == p && !(seen[k >> 3] & (1 << (k & 7)))) break;
    wfound = (k < wn);
}

__attribute__((noinline)) void locate(unsigned char v) {
    unsigned char r, c = 0;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++)
            if (grid[r][c] == v && ((r << 2) | c) != (v & 15) + 16) break;
        if (c < cols) break;
    }
    at = (r < rows) ? (unsigned char)((r << 4) | c) : 0xFF;
}

__attribute__((noinline)) unsigned char scan(const unsigned char *s, unsigned char stop) {
    unsigned char i = 0, lim = n;
    while (i < lim) {
        if ((unsigned char)((s[i] << 1) ^ (s[i] >> 7)) == stop) break;
        i++;
    }
    return i < lim ? i : 0x80;
}

int main(void) {
    unsigned char ok;

    n = 4; par[0] = 9; par[1] = 9; par[2] = 0; par[3] = 9;
    find(0);
    ok = found == 1;
    find(7);
    ok = ok && found == 0;
    seen[0] = 4;                 /* entry 2 seen: not found */
    find(0);
    ok = ok && found == 0;
    seen[0] = 0;
    OUT[0] = ok;

    wn = 4; wpar[0] = 900; wpar[1] = 900; wpar[2] = 0x1234; wpar[3] = 900;
    wfind(0x1234);
    ok = wfound == 1;
    wfind(77);
    ok = ok && wfound == 0;
    OUT[1] = ok;

    rows = 3; cols = 4;
    grid[1][2] = 0x21; grid[2][3] = 0x33;
    locate(0x21);
    ok = at == 0x12;
    locate(0x33);
    ok = ok && at == 0x23;
    locate(0x44);
    ok = ok && at == 0xFF;
    OUT[2] = ok;

    par[0] = 1; par[1] = 2; par[2] = 0x81; par[3] = 4;
    ok = scan(par, 3) == 2 && scan(par, 8) == 3 && scan(par, 99) == 0x80;
    OUT[3] = ok;

    OUT[4] = 0xAA;
    return 0;
}
