/* test_float_literal_xemu.c — floating constants without digits on one side
 * of the point: .5, .5e1, .125f, 1., 2.f, 1.e2 (C11 6.4.4.2); member access
 * and the ... of case ranges unchanged
 *
 * Results at $C0C0 (01 = pass):
 *   [0] .5, .5e1, .125f, -.75   [1] x * .25, a macro .5, 1. + .0, 1., 2.f, 1.e2
 *   [2] v.a, p->a, case 1 ... 3, case 4...5   [3] sizeof .5 / 1. is a double's
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define H .5
struct s { int a; } v = { 3 };
float f(float x) { return x * .25; }
int g(int n) { switch (n) { case 1 ... 3: return 1; case 4...5: return 2; } return 0; }
int main(void) {
    float a = .5, b = .5e1, c = .125f, d = -.75, e = 1., h = 2.f, k = 1.e2;
    struct s *p = &v;
    OUT[0] = a == 0.5 && b == 5.0 && c == 0.125 && d == -0.75;
    OUT[1] = f(8) == 2.0 && H + .5 == 1.0 && 1. + .0 == 1.0 && e == 1.0 && h == 2.0 && k == 100.0;
    OUT[2] = v.a == 3 && p->a == 3 && g(2) == 1 && g(5) == 2 && g(6) == 0;
    OUT[3] = sizeof .5 == sizeof(double) && sizeof 1. == sizeof(double);
    OUT[4] = 0xAA;
    return 0;
}
