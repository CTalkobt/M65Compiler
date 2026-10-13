/* test_complex_xemu.c — _Complex without <complex.h>
 *
 * Results at $C0C0 (01 = pass):
 *   [0] constant global initializer (1.0 + 1.0i), int complex global
 *   [1] real + imaginary at run time, unary minus, == / !=
 *   [2] casts of real values to complex ((_Complex double)0.0)
 *   [3] by-value complex parameters: x + y, x += y, return
 *   [4] real op complex: 5.0 * (1.0fi), 1.0f / z
 *   [5] _Complex int <-> _Complex float conversion (assignment, compare)
 *   [6] conjugate (~) of float and int complex
 *   [7] 4-byte struct compound literal assigned by value
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

__complex__ double ag = 1.0 + 1.0i;
_Complex int ig = 3 + 4.0i;

__complex__ double add_assign(__complex__ double x, __complex__ double y) { x += y; return x; }
__complex__ double add(__complex__ double x, __complex__ double y) { return x + y; }
_Complex float conj_f(_Complex float x) { return ~x; }
_Complex int conj_i(_Complex int x) { _Complex int r; r = ~x; return r; }

struct S { signed int a, b; };

int main(void) {
    __complex__ double a, b, z;
    _Complex int i;
    struct S s;

    OUT[0] = __real__ ag == 1.0 && __imag__ ag == 1.0 && __real__ ig == 3 && __imag__ ig == 4;

    a = ag;
    b = -2.0 + 2.0i;
    OUT[1] = __real__ b == -2.0 && __imag__ b == 2.0 && a == a && a != b && -b == 2.0 - 2.0i;

    z = (__complex__ double) 0.0;
    OUT[2] = __real__ z == 0.0 && __imag__ z == 0.0 &&
             (__complex__ double) 0.0 == (__complex__ double) (-0.0) && (_Complex double) 2 == 2.0;

    OUT[3] = add(a, b) == -1.0 + 3.0i && add_assign(a, b) == -1.0 + 3.0i;

    z = 1.0 + 5.0 * (1.0fi);
    OUT[4] = z == 1.0 + 5.0i && 2.0 / (1.0 + 1.0i) == 1.0 - 1.0i;

    i = 1.0 - 2.0i;
    z = i;
    OUT[5] = __real__ i == 1 && __imag__ i == -2 && z == 1.0 - 2.0i && i == 1.0 - 2.0i && i != 1.0 + 2.0i;

    OUT[6] = conj_f(1.0 + 2.0i) == 1.0 - 2.0i && conj_i(i) == 1 + 2.0i;

    s = (struct S){1, -2};
    OUT[7] = s.a == 1 && s.b == -2;

    OUT[8] = 0xAA;
    return 0;
}
