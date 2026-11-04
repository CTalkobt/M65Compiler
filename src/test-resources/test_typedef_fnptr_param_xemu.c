/* test_typedef_fnptr_param_xemu.c — parameters of a typedef'd function-pointer
 * or function type without a name in a prototype (#402: void g(fp);), and
 * pointers to such function pointers (fp *)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int apply(fp, int); declared unnamed, defined named, called
 *   [1] const fp, unnamed, among other parameters
 *   [2] fp *: a pointer to a function pointer, called through (*pp)(v) and pp[1](v)
 *   [3] typedef int fn(int): a parameter of the function type (its pointer), and fn *
 *   [4] sizes: fp, fp *, fn *, an array of fp, a global fp *
 *   [5] struct members fp, fp *, fn *
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef int (*fp)(int);
typedef int fn(int);

static int inc(int x) { return x + 1; }
static int dbl(int x) { return x * 2; }

int apply(fp, int);
int apply2(char, const fp, int);
int applyp(fp *, int);
int applyf(fn, int);

int apply(fp f, int v) { return f(v); }
int apply2(char k, const fp f, int v) { return f(v) + k; }
int applyp(fp *pp, int v) { return (*pp)(v) + pp[1](v); }
int applyf(fn f, int v) { return f(v); }

fp table[2] = {inc, dbl};
struct holder { fp a; fp *b; fn *c; char z; };
fp *tp = table;

int main(void) {
    fn *q = dbl;
    OUT[0] = apply(inc, 41) == 42;
    OUT[1] = apply2(3, dbl, 10) == 23;
    OUT[2] = applyp(table, 5) == 6 + 10 && (*tp)(1) == 2 && tp[1](4) == 8;
    OUT[3] = applyf(inc, 9) == 10 && q(21) == 42 && applyf(q, 3) == 6;
    OUT[4] = sizeof(fp) == 2 && sizeof(fp *) == 2 && sizeof(fn *) == 2 && sizeof(table) == 4 && sizeof(tp) == 2 && sizeof(*tp) == 2;
    {
        struct holder h;
        h.a = inc; h.b = table; h.c = dbl;
        OUT[5] = sizeof(struct holder) == 7 && h.a(1) == 2 && (*h.b)(2) == 3 && h.b[1](3) == 6 && h.c(4) == 8;
    }
    OUT[6] = 0xAA;
    return 0;
}
