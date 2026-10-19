/* test_bool_ptr_xemu.c — a pointer as an operand of && / || (Issue #309):
 * its truth value is all of it (a function before it whose && result had
 * the same vreg number made it its low byte: false at $xx00)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a && b, both pointers, one at $6200
 *   [1] k && b && c, the last a pointer at $6200 (the issue's case)
 *   [2] a && *a == 'x' && b
 *   [3] a || b, either one at $6200
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

NOINLINE static int both(char *a, char *b) { return a && b; }
NOINLINE static int three(int k, char *b, char *c) { return k && b && c; }
NOINLINE static int with_cmp(char *a, char *b) { return a && *a == 'x' && b; }
NOINLINE static int either(char *a, char *b) { return a || b; }

int main(void) {
    char *p = (char *)0x6200, *q = (char *)0x6201;
    static char x[] = "x";
    OUT[0] = both(p, q) == 1 && both(q, p) == 1 && both(0, p) == 0;
    OUT[1] = three(1, p, p) == 1 && three(1, q, 0) == 0 && three(0, p, p) == 0;
    OUT[2] = with_cmp(x, p) == 1 && with_cmp(x, 0) == 0;
    OUT[3] = either(0, p) == 1 && either(p, 0) == 1 && either(0, 0) == 0;
    OUT[4] = 0xAA;
    return 0;
}
