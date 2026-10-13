/* test_stmt_expr_xemu.c — GCC statement expressions ({ ...; expr; })
 *
 * Results at $C0C0 (01 = pass):
 *   [0] block-local variable as the value
 *   [1] MAX-style macro evaluates its argument once
 *   [2] assignment inside, value is the last expression
 *   [3] loop inside the block
 *   [4] statement expression without a value (side effects only)
 *   [5] long value
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
static int calls;
static int bump(void) { return ++calls; }
#define MAX(a,b) ({ int _a = (a), _b = (b); _a > _b ? _a : _b; })
int main(void) {
    int x = 5, y;
    y = ({ int t = x * 2; t + 1; });
    OUT[0] = y == 11;
    OUT[1] = MAX(bump(), 0) == 1 && calls == 1;
    OUT[2] = ({ x = 7; x; }) == 7 && x == 7;
    y = ({ int i, s = 0; for (i = 0; i < 4; i++) s += i; s; });
    OUT[3] = y == 6;
    ({ x = 9; });
    OUT[4] = x == 9;
    long l = ({ long v = 70000L; v + 1; });
    OUT[5] = l == 70001L;
    OUT[6] = 0xAA;
    return 0;
}
