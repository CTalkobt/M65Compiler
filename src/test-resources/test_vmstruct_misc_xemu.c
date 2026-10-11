/* test_vmstruct_misc_xemu.c — structs with run-time member sizes and
 * assorted single fixes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] struct { T a[n]; int b[n]; }: sizeof, member offsets, indexing
 *   [1] pointer to such a struct: p[i] stride, struct copy, statement
 *       expression value; by value through ... and va_arg(ap, typeof(v))
 *   [2] typedef int T[n] in a struct; offsetof with run-time strides
 *   [3] a stack frame over 255 bytes (parameters past offset 255)
 *   [4] __int128 from int (initializer, op=, argument); L"..." wide string
 *   [5] _Complex members initialized from complex constants; f().m of a
 *       struct returned in registers; &&a - &&b in a static initializer
 *   [6] #pragma push_macro/pop_macro
 *   [7] $AA sentinel
 */
#include <stdarg.h>
#include <stddef.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static int layout(int n) {
    struct S { struct T { short c; } a[n]; int b[n]; } s;
    int i, ok;
    for (i = 0; i < n; i++) { s.a[i].c = 0x1234 + i; s.b[i] = -i; }
    ok = sizeof(s) == 4 * n && (char *)&s.b[0] - (char *)&s == 2 * n;
    for (i = 0; i < n; i++) ok = ok && s.a[i].c == 0x1234 + i && s.b[i] == -i;
    return ok;
}

static int sum_va(int n, ...) {
    struct V { char x[n]; } v;
    va_list ap;
    int i, s = 0;
    va_start(ap, n);
    v = va_arg(ap, typeof(v));
    va_end(ap);
    for (i = 0; i < n; i++) s += v.x[i];
    return s;
}

static int copies(char *buf, int y) {
    struct S { char w[y]; } *p = (void *)buf, a;
    int i;
    a = ({ struct S b; b = p[2]; p[3] = b; });
    for (i = 0; i < y; i++) if (a.w[i] != buf[2 * y + i] || buf[3 * y + i] != buf[2 * y + i]) return 0;
    return (char *)&p[1] - (char *)p == y && sum_va(y, a) == 2 * y * y + y * (y - 1) / 2;
}

static int vla_typedef(int n, int i, int j) {
    typedef int T[n];
    struct S { int a; T b[n]; };
    return __builtin_offsetof(struct S, b[i][j]) == 2 + (n * i + j) * 2;
}

typedef struct { char x[72]; } Big;
static int big_frame(int first, ...) {
    Big a, b, c, d;
    va_list ap;
    int v;
    va_start(ap, first);
    v = va_arg(ap, int);
    va_end(ap);
    a.x[71] = 1; d.x[70] = 2;
    return first == 21 && v == 22 && a.x[71] + d.x[70] == 3;
}

typedef unsigned __int128 u128;
static int wide_arg(u128 x) { return ((unsigned char *)&x)[0] == 7 && ((unsigned char *)&x)[15] == 0; }

struct C { _Complex char a; _Complex char b; } cs = { 1 + 2i, 3 + 4i };
struct R { _Complex unsigned short x; } gr = { 100 + 200i };
static struct R get_r(void) { return gr; }

static int label_diff(int x) {
    static int t[] = { &&l1 - &&l0, &&l2 - &&l0 };
    int c = 0;
    goto *(&&l0 + t[x]);
l1:
    c += 2;
l2:
    c++;
l0:
    return c;
}

#define PM 2
#pragma push_macro("PM")
#undef PM
#define PM 1
#pragma pop_macro("PM")

int main(void) {
    char buf[32];
    int i;
    OUT[0] = layout(5);
    for (i = 0; i < 32; i++) buf[i] = i;
    OUT[1] = copies(buf, 8);
    OUT[2] = vla_typedef(5, 2, 3) && vla_typedef(4, 1, 1);
    OUT[3] = big_frame(21, 22);
    {
        u128 a = 0, b = 5;
        unsigned char *pa = (unsigned char *)&a;
        unsigned int *w = L"ab";
        a -= 1;
        OUT[4] = pa[0] == 0xFF && pa[15] == 0xFF && ((unsigned char *)&b)[0] == 5 &&
                 wide_arg(7) && w[1] == 'b' && sizeof(L"ab") == 6;
    }
    OUT[5] = __real__ cs.a == 1 && __imag__ cs.b == 4 && get_r().x == gr.x && __real__ get_r().x == 100 &&
             label_diff(0) == 3 && label_diff(1) == 1;
    OUT[6] = PM == 2;
    OUT[7] = 0xAA;
    return 0;
}
