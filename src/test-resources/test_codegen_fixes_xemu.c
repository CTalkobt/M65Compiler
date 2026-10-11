/* test_codegen_fixes_xemu.c — assorted code generation fixes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] variable shifts (i >> k, i << k with both in variables, 16 and 32 bit)
 *   [1] signed char -> long / unsigned short; long vs unsigned short compare
 *   [2] "text"[i]; i[p]; f() + 1 on a returned pointer; -(x < y) is -1
 *   [3] ++*s++ and prefix/postfix ++/-- on *p, a[i]
 *   [4] int a[][M] and char a[2][n] parameters; i[p] subscripts
 *   [5] variable-length local arrays (1-D, 2-D, sizeof)
 *   [6] struct compound literals ((A){...}.a), &(int[]){...}[++a],
 *       &by-value struct parameter, static locals (value and address)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct { unsigned short a; } A;
struct S { int a, b, c; };

static signed int shr(signed int i, int k) { return i >> k; }
static unsigned shl(unsigned i, int k) { return i << k; }
static signed long shrl(signed long i, int k) { return i >> k; }
static signed int lt(signed int x, signed int y) { return -(x < y); }
int buf[8];
static int *first(void) { return buf; }
static int sum2(int a[][3], int n) { int s = 0, i, j; for (i = 0; i < n; i++) for (j = 0; j < 3; j++) s += a[i][j]; return s; }
int N = 1;
static void fill(char a[2][N]) { a[1][0] = N; a[1][1] = 7; }
static int vla(int r, int c) {
    char g[r][c];
    int i, j;
    for (i = 0; i < r; i++) for (j = 0; j < c; j++) g[i][j] = i * 10 + j;
    return (g[r - 1][c - 1] + g[1][0]) * 100 + (int)sizeof(g);
}
static int vla1(int n) { int a[n], i, s = 0; for (i = 0; i < n; i++) a[i] = i + 1; for (i = 0; i < n; i++) s += a[i]; return s + (int)sizeof(a); }
static int byval(struct S y) { struct S *p = &y; return p->b; }
static int counter(void) { static int n = 5; int *pn = &n; (*pn)++; return n; }

int main(void) {
    signed char sa = -30, sb = -31;
    signed long la = sa;
    char str[4] = "000";
    char *s = str;
    int arr[3] = {10, 20, 30};
    int m[2][3] = {{1, 2, 3}, {4, 5, 6}};
    char cb[10] = {0};
    int a = 0, *p;
    struct S st = {1, 42, 3};
    unsigned v;
    int i = 1;

    OUT[0] = shr(9766, 11) == 4 && shl(3, 4) == 48 && shr(-64, 3) == -8 && shrl(0x123456L, 16) == 0x12;

    OUT[1] = la == -30 && (unsigned short)sb == 0xFFE1 && !((signed long)sa > (unsigned short)sb);

    OUT[2] = "foobar"[3] == 'b' && 2[arr] == 30 && first() + 1 == &buf[1] && lt(5, 7) == -1 && lt(7, 5) == 0;

    ++*s++;
    p = &arr[1];
    (*p)++; ++arr[2]; --*p;
    OUT[3] = str[0] == '1' && s == str + 1 && arr[1] == 20 && arr[2] == 31;

    N = 4;
    fill((void *)cb);
    OUT[4] = sum2(m, 2) == 21 && cb[4] == 4 && cb[5] == 7;

    OUT[5] = vla1(5) == 25 && vla(3, 4) == 3312;

    v = ((A){ ((!(i >> 4) ? 8 : 64) << 8) + (i << 4) }).a;
    p = &(int []){0, 1, 2}[++a];
    OUT[6] = v == 0x810 && a == 1 && *p == 1 && byval(st) == 42 && counter() == 6 && counter() == 7;

    OUT[7] = 0xAA;
    return 0;
}
