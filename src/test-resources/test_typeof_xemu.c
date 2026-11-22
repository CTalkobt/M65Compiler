/* test_typeof_xemu.c — #707: typeof keeps a file-scope variable's
 * signedness, and is accepted wherever a type is (file scope, a cast, a
 * for declaration, a function's result); typeof(&x), typeof(*p),
 * typeof(p[i]) and typeof of a cast give their types.
 *
 * Results at $C0C0 (01 = pass, $AA sentinel):
 *   [0] typeof(global signed char) is signed  [1] typeof(local signed char)
 *   [2] typeof at file scope                   [3] typeof in a cast
 *   [4] typeof in a for declaration            [5] typeof(&g), (*p), (p[0])
 *   [6] typeof(type), multi-declarations       [7] a typeof function result
 */
#define OUT ((volatile unsigned char *)0xC0C0)

signed char g = -3;
signed char ga = -1, gb = -2;          /* gb: the second of a group */
signed int gi = -300;
typeof(g) g2 = -5;                     /* file scope */
typeof(gi) gi2 = -1000;

typeof(g) neg7(void) { return -7; }

volatile signed char vg = -4;

int main(void) {
    signed char l = -3;

    { typeof(g) b = g; OUT[0] = (b == -3 && b < 0) ? 1 : 0; }
    { typeof(l) c = l; OUT[1] = (c == -3 && c < 0) ? 1 : 0; }

    OUT[2] = (g2 == -5 && g2 < 0 && gi2 == -1000 && gi2 < 0 && sizeof(g2) == 1 && sizeof(gi2) == 2) ? 1 : 0;

    {
        unsigned char raw = 0xFE;
        signed int v = (typeof(g))raw;        /* -2 as a signed char */
        signed int w = (typeof(vg))vg;
        OUT[3] = (v == -2 && w == -4) ? 1 : 0;
    }

    {
        unsigned char n = 0;
        for (typeof(g) i = -2; i < 2; i++) n++;
        OUT[4] = n == 4 ? 1 : 0;
    }

    {
        typeof(&g) q = &g;             /* signed char * */
        signed char *p = &g;
        typeof(*p) d = *p;
        typeof(p[0]) e = p[0];
        OUT[5] = (*q == -3 && d == -3 && e == -3 && d < 0 && e < 0 && sizeof(q) == 2 && sizeof(d) == 1) ? 1 : 0;
    }

    {
        typeof(signed char *) r = &g;
        typeof(signed char) s = -9;
        typeof(gb) t = gb;
        typeof(ga) u = ga;
        OUT[6] = (*r == -3 && s == -9 && s < 0 && t == -2 && t < 0 && u == -1) ? 1 : 0;
    }

    { signed int x = neg7(); OUT[7] = (x == -7) ? 1 : 0; }

    OUT[8] = 0xAA;
    return 0;
}
