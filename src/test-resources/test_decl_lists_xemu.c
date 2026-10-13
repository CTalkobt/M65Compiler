/* test_decl_lists_xemu.c — function declarators in a declaration list
 *
 * Results at $C0C0 (01 = pass):
 *   [0] static int f(int), g(void), *h(void);   (prototypes, one static list)
 *   [1] int k(void), y1;                        (function, then a variable)
 *   [2] int y2 = 5, k2(void);                   (variable, then a function)
 *   [3] int z, q(int);                          (the same in a block)
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
static int f(int), g(void), *h(void);
int x1 = 3;
int k(void), y1;
int y2 = 5, k2(void);

int main(void) {
    int z, q(int);
    y1 = 4;
    z = 2;
    OUT[0] = f(1) == 2 && g() == 7 && *h() == 3;
    OUT[1] = k() == 9 && y1 == 4;
    OUT[2] = k2() == 6;
    OUT[3] = q(z) == 2;
    OUT[4] = 0xAA;
    return 0;
}

static int f(int a) { return a + 1; }
static int g(void) { return 7; }
static int *h(void) { return &x1; }
int k(void) { return 9; }
int k2(void) { return y2 + 1; }
int q(int a) { return a; }
