/* test_parse_fixes_xemu.c — the comma operator in for clauses, and macro
 * arguments that contain a parameter's name
 *
 * Results at $C0C0 (01 = pass):
 *   [0] for (i = 0, j = 10; i < 5; i++, j--)
 *   [1] a comma expression as a for condition
 *   [2] F(dst, src, len) called with src = g(len): the argument's "len" is
 *       not the parameter (each parameter is substituted once)
 *   [3] SWAP(a, b) -> b a called as SWAP(y, x); a variadic argument naming
 *       the macro's parameter
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int g(unsigned int v) { return v * 2; }
static unsigned int three(unsigned int a, unsigned int b, unsigned int c) { return a * 100 + b * 10 + c; }
#define F(dst, src, len) three((dst), (src), (len))
#define MINUS(a, b) ((a) - (b))
#define SWAP(a, b) MINUS(b, a)
#define FIRST(fmt, ...) three(fmt, __VA_ARGS__)

int main(void) {
    unsigned char i, j, n = 0;
    unsigned int len = 3, x = 9, y = 4, fmt = 1;

    for (i = 0, j = 10; i < 5; i++, j--) n += j;
    OUT[0] = n == 40 && i == 5 && j == 5;

    for (i = 0; n = n + 1, i < 3; i++) ;
    OUT[1] = n == 44;

    OUT[2] = F(1, g(len), 2) == 100 + 60 + 2;      /* g(3) = 6, not g(2) */

    OUT[3] = SWAP(y, x) == 5 && FIRST(2, fmt, 0) == 210;

    OUT[4] = 0xAA;
    return 0;
}
