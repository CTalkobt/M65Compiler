/* test_extern_sizeof_xemu.c — sizeof of a variable in static initializers
 * (Issue #312): the arrays are extern here, defined in
 * test_extern_sizeof_b.c. The initializers used to be emitted as zeros
 * (and the table too short); sizeof of any variable in them was lost.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a table of { address, sizeof } pairs (the issue's case)
 *   [1] a scalar: unsigned x = sizeof m
 *   [2] an array: { sizeof m, sizeof m / sizeof m[0], sizeof lv }
 *   [3] a local static and an expression: (unsigned)sizeof m - 1
 *   [4] sizeof of a struct variable and of one of its members
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct pt { int x; long y; unsigned char tag[3]; };

extern unsigned int m[39];
extern long lv[3];
extern struct pt where;
unsigned char b[4];

struct var { void *p; unsigned char n; };
static const struct var vars[] = { { b, 4 }, { m, sizeof m }, { lv, sizeof lv } };
unsigned x = sizeof m;
unsigned arr[] = { sizeof m, sizeof m / sizeof m[0], sizeof lv };
unsigned expr = (unsigned)sizeof m - 1;
unsigned char sizes[] = { sizeof where, sizeof where.tag };

static unsigned local_static(void) {
    static unsigned q = sizeof m;
    return q;
}

int main(void) {
    OUT[0] = sizeof vars == 9 && vars[0].p == b && vars[0].n == 4 && vars[1].p == m && vars[1].n == 78 &&
             vars[2].p == lv && vars[2].n == 12;
    OUT[1] = x == 78;
    OUT[2] = sizeof arr == 6 && arr[0] == 78 && arr[1] == 39 && arr[2] == 12;
    OUT[3] = local_static() == 78 && expr == 77;
    OUT[4] = sizes[0] == 9 && sizes[1] == 3;
    OUT[5] = 0xAA;
    return 0;
}
