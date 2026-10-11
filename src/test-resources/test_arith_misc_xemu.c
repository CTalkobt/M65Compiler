/* test_arith_misc_xemu.c — pointer compound assignment, float compound
 * assignment and initialization, nested function static chain, variably
 * modified parameters
 *
 * Results at $C0C0 (01 = pass):
 *   [0] p += n / p -= n scale by the element size (int, long, float, struct)
 *   [1] *(p + i) on a float pointer        [2] float += -= *= /=, int += float
 *   [3] float globals/locals initialized from integers
 *   [4] nested function: parent locals through the static chain, struct result
 *   [5] sub(int i, int a[i++]) evaluates i++ on entry
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct T { char a, b, c; };
int ai[8]; long al[8]; float af[8]; struct T at[8];
float gf = 10;
struct A { int one, two, three; };

static int nested(void) {
    int base;
    struct A Foo(void) {
        struct A a;
        a.one = base + 1; a.two = base + 2; a.three = base + 3;
        return a;
    }
    int Bar(void) { return base + 7; }
    base = 10;
    struct A a = Foo();
    return Bar() == 17 && a.one == 11 && a.two == 12 && a.three == 13;
}

int vla(int i, int array[i++]) { return i; }

int main(void) {
    int *pi = ai; long *pl = al; float *pf = af; struct T *pt = at;
    int x = 1, k = 2;
    float c = 1.5f, d = 10, l = 3;
    int n = 7;
    float u = k;
    pi += 3; pl += 3; pf += 3; pt += 3; pl -= 1;
    OUT[0] = (pi - ai) == 3 && (pl - al) == 2 && (pf - af) == 3 && (pt - at) == 3 &&
             ((char *)pf - (char *)af) == 15;
    af[5] = 2.0f;
    pf = af + 4;
    OUT[1] = (int)*(pf + x) == 2 && (int)pf[x] == 2;
    c += 2.0f; c *= 2; d -= af[5]; d /= 4; n += 2.5f;
    OUT[2] = c == 7.0f && d == 2.0f && n == 9;
    OUT[3] = gf == 10.0f && l == 3.0f && u == 2.0f;
    OUT[4] = nested();
    OUT[5] = vla(10, ai) == 11;
    OUT[6] = 0xAA;
    return 0;
}
