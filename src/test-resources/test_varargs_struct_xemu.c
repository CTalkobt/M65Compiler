/* test_varargs_struct_xemu.c — structs through '...', member arrays,
 * stack-frame loops, signed int -> float conversion
 *
 * Results at $C0C0 (01 = pass):
 *   [0] 3- and 7-byte structs through '...' mixed with an int
 *   [1] 15-byte struct (float array member) twice, read in a loop/switch
 *   [2] 2- and 4-byte structs through '...' (passed as values)
 *   [3] 2-D array members: row addresses, element read/write, through ->
 *   [4] ++i / --i on a local of a stack-frame (variadic) function
 *   [5] signed long / negative literal -> float in assignment & initializer
 *   [6] $AA sentinel
 */
#include <stdarg.h>
#define OUT ((volatile unsigned char *)0xC0C0)

struct S3 { char a, b, c; };
struct S7 { long x; int y; char z; };
struct F { float a[3]; };
struct S2 { char a, b; };
struct S4 { int a, b; };
struct M { int f[3]; double x[2][2]; char c[2][3]; int m[3][2]; };

struct F fa[3];
struct M mm, *pm = &mm;
volatile signed long neg = -49026L;

static int sum(int n, ...) {
    va_list ap; struct S3 s; struct S7 l; int r;
    va_start(ap, n);
    s = va_arg(ap, struct S3);
    r = va_arg(ap, int);
    l = va_arg(ap, struct S7);
    va_end(ap);
    return s.a + s.b + s.c + r + (int)l.x + l.y + l.z;
}

static int floats(int z, ...) {
    va_list ap; struct F arg; int i, ok = 0;
    va_start(ap, z);
    for (i = 2; i < 4; ++i) {
        switch (i) {
        case 2: case 3: arg = va_arg(ap, struct F); break;
        default: return 0;
        }
        if (arg.a[2] == fa[1].a[2] && arg.a[0] == 1.5) ok++;
    }
    va_end(ap);
    return ok == 2;
}

static int small(int n, ...) {
    va_list ap; struct S2 a; struct S4 b;
    va_start(ap, n);
    a = va_arg(ap, struct S2);
    b = va_arg(ap, struct S4);
    va_end(ap);
    return a.a == 1 && a.b == 2 && b.a == 300 && b.b == -4;
}

static int counter(int n, ...) {
    va_list ap; int i, k = 0;
    va_start(ap, n);
    for (i = 0; i < n; ++i) k += va_arg(ap, int);
    for (i = n; i > 0; --i) k++;
    va_end(ap);
    return k;
}

int main(void) {
    struct S3 s = {1, 2, 3};
    struct S7 l = {100, 20, 4};
    struct S2 s2 = {1, 2};
    struct S4 s4 = {300, -4};
    char *b = (char *)&mm;
    float f = -49026, g;
    int i, j;

    OUT[0] = sum(0, s, 1000, l) == 1130 && sum(0, s, 7, l) == 137;

    fa[1].a[0] = 1.5;
    fa[1].a[2] = -49026;
    OUT[1] = floats(1, fa[1], fa[1]);

    OUT[2] = small(0, s2, s4);

    for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) mm.x[i][j] = i * 2 + j;
    for (i = 0; i < 3; i++) for (j = 0; j < 2; j++) pm->m[i][j] = i * 10 + j;
    pm->c[1][2] = 'z';
    OUT[3] = (char *)mm.x[1] - b == 16 && (char *)mm.c[1] - b == 29 &&
             mm.x[1][0] == 2.0 && pm->x[0][1] == 1.0 && mm.m[2][1] == 21 && pm->m[1][0] == 10 &&
             mm.c[1][2] == 'z';

    OUT[4] = counter(3, 10, 20, 30) == 63;

    g = neg;
    OUT[5] = f == -49026.0 && g == -49026.0 && f == -49026;
    OUT[6] = 0xAA;
    return 0;
}
