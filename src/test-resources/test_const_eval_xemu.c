/* test_const_eval_xemu.c — constant expressions in static initializers and
 * case labels, through ConstEval (Issue #93)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] initializers with sizeof and <<, %, ?:, == and &&, which were
 *       "not a compile-time constant"
 *   [1] a cast's type: (signed char)200 is -56; (unsigned)-1 in a long is
 *       65535, not -1
 *   [2] a braced list and a struct with those expressions, a local static
 *   [3] case sizeof x and a range ending at sizeof(long) (they were case 0)
 *   [4] case (signed char)200 on a signed char selector
 *   [5] float f = 7 / 2 is 3 (an integer division), float g = 7 / 2.0 is 3.5
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int x;
long lx;
struct pair { unsigned a; signed char b; };

unsigned q1 = sizeof x << 1;
unsigned q2 = sizeof lx > 2 ? 7 : 9;
unsigned q3 = (sizeof x == 2) && (sizeof lx == 4);
unsigned q4 = sizeof x % 3;
signed int n2 = (signed char)200;
long l1 = (unsigned)-1;
unsigned list[] = { sizeof lx << 2, sizeof x * 3 % 4, sizeof(long) == 4 ? 11 : 12 };
struct pair pr = { sizeof lx * 10, (signed char)200 };
float f1 = 7 / 2;
float f2 = 7 / 2.0;

static unsigned local_static(void) {
    static unsigned q = sizeof lx << 3;
    return q;
}

static unsigned char kind(int v) {
    switch (v) {
    case sizeof x: return 1;            /* 2 */
    case 3 ... sizeof(long) + 1: return 2;   /* 3..5 */
    case 0: return 3;
    }
    return 0;
}

static unsigned char skind(signed char c) {
    switch (c) {
    case (signed char)200: return 1;    /* -56 */
    case 56: return 2;
    }
    return 0;
}

int main(void) {
    OUT[0] = q1 == 4 && q2 == 7 && q3 == 1 && q4 == 2;
    OUT[1] = n2 == -56 && l1 == 65535L;
    OUT[2] = list[0] == 16 && list[1] == 2 && list[2] == 11 && pr.a == 40 && pr.b == -56 && local_static() == 32;
    OUT[3] = kind(2) == 1 && kind(3) == 2 && kind(5) == 2 && kind(0) == 3 && kind(6) == 0;
    OUT[4] = skind(-56) == 1 && skind(56) == 2 && skind(0) == 0;
    OUT[5] = f1 == 3.0 && f2 == 3.5;
    OUT[6] = 0xAA;
    return 0;
}
