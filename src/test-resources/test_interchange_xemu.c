/* test_interchange_xemu.c — loop interchange (-floop-interchange) keeps
 * results, both for nests it swaps and for nests it must leave alone.
 * test_xemu.sh builds it with -floop-interchange (and without);
 * test_optimizations.sh checks with -Roptir which nests were swapped:
 * colcopy and colsum only.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] colcopy: b[j][i] = a[j][i] + 1 over i (outer), j (inner) — swapped
 *   [1] colsum: an integer reduction over a column-order nest — swapped
 *   [2] dep: a[j][i] = a[j-1][i] + 1 (loop-carried dependence) — kept
 *   [3] triangular: inner bound j < i — kept; break in the body — kept
 *   [4] call in the body (order of calls is observable) — kept
 *   [5] loop variable used after the nest — kept; store through a pointer — kept
 *   [6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define R 4
#define C 5

int a[R][C], b[R][C];
int order[R * C];
int norder;

static void note(int v) { order[norder++] = v; }

static void fill(void) {
    int r, c;
    for (r = 0; r < R; r++)
        for (c = 0; c < C; c++)
            a[r][c] = r * 10 + c;
}

static int colcopy(void) {
    int r, c;
    for (int i = 0; i < C; i++)
        for (int j = 0; j < R; j++)
            b[j][i] = a[j][i] + 1;
    for (r = 0; r < R; r++)
        for (c = 0; c < C; c++)
            if (b[r][c] != r * 10 + c + 1) return 0;
    return 1;
}

static int colsum(void) {
    int s = 0;
    for (int i = 0; i < C; i++)
        for (int j = 0; j < R; j++)
            s += a[j][i];
    /* sum of r*10 + c over 4 rows and 5 columns: 5*10*(0+1+2+3) + 4*(0+1+2+3+4) */
    return s == 340;
}

static int dep(void) {
    int r;
    fill();
    for (int i = 0; i < C; i++)
        for (int j = 1; j < R; j++)
            a[j][i] = a[j - 1][i] + 1;
    for (r = 0; r < R; r++)
        if (a[r][2] != 2 + r) return 0;
    return 1;
}

static int triangular_and_break(void) {
    int s = 0, t = 0;
    fill();
    for (int i = 0; i < C; i++)
        for (int j = 0; j < i && j < R; j++)
            s += a[j][i];
    /* i=1: a[0][1]=1; i=2: 2+12; i=3: 3+13+23; i=4: 4+14+24+34 (130) */
    for (int i = 0; i < C; i++)
        for (int j = 0; j < R; j++) {
            if (a[j][i] == 22) break;
            t += a[j][i];
        }
    /* every column summed except a[2][2] and a[3][2] (break at 22) */
    return s == 130 && t == 340 - 22 - 32;
}

static int calls(void) {
    norder = 0;
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++)
            note(a[j][i]);
    /* column order: a[0][0], a[1][0], a[0][1], a[1][1] */
    return norder == 4 && order[0] == 0 && order[1] == 10 && order[2] == 1 && order[3] == 11;
}

static int after_and_pointer(void) {
    int i, j, ok;
    int *p = &b[0][0];
    for (i = 0; i < C; i++)
        for (j = 0; j < R; j++)
            b[j][i] = 7;
    ok = i == C && j == R;
    for (int x = 0; x < C; x++)
        for (int y = 0; y < R; y++)
            p[y * C + x] = a[y][x];
    return ok && b[2][3] == a[2][3] && b[3][4] == a[3][4];
}

int main(void) {
    fill();
    OUT[0] = colcopy();
    OUT[1] = colsum();
    OUT[2] = dep();
    OUT[3] = triangular_and_break();
    OUT[4] = calls();
    OUT[5] = after_and_pointer();
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
