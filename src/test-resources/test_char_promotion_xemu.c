/* test_char_promotion_xemu.c — chars in int arithmetic and as int arguments
 *
 * + - * << of chars are ints: a char operand added to such a sum must not
 * cut it back to 8 bits (a + (b + c) lost the carry of b + c, 600 -> 344).
 * A narrower argument widens to its parameter by its own signedness: an
 * unsigned char 200 passed to a signed int is 200 (with -fsigned-default
 * it was sign-extended to -56); a signed char stays negative; a constant
 * keeps the parameter's signedness (-1 to a signed long is -1).
 * Run in both conventions, with and without -fsigned-default.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sums of three and four chars, nested and from calls
 *   [1] - * << of chars inside a wider sum
 *   [2] unsigned char arguments to signed int / signed long parameters
 *   [3] signed char and constant arguments to wider signed parameters
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

volatile unsigned char a = 200, b = 200, c = 200, d = 112;
volatile signed char sc = -5;
static unsigned char v[4] = {112, 112, 112, 112};

N static unsigned char get(unsigned char i) { return v[i]; }
N static signed int as_int(signed int x) { return x; }
N static signed long as_long(signed long x) { return x; }
N static signed int two(unsigned char k, signed int x) { return x + k - k; }

int main(void) {
    unsigned int s;
    unsigned char ok0 = 1, ok1 = 1, ok2 = 1, ok3 = 1;

    s = a + b + c;                          if (s != 600) ok0 = 0;
    s = a + (b + c);                        if (s != 600) ok0 = 0;
    s = (a + b) + (b + c);                  if (s != 800) ok0 = 0;
    s = get(0) + get(1) + get(2) + get(3);  if (s != 448) ok0 = 0;
    s = v[0] + v[1] + v[2] + v[3];          if (s != 448) ok0 = 0;

    s = a + (b * 2);                        if (s != 600) ok1 = 0;
    s = d + (b << 1);                       if (s != 512) ok1 = 0;
    s = 1000 + (d - a) + a;                 if (s != 1112) ok1 = 0;
    s = a + b * c;                          if (s != 40200) ok1 = 0;

    if (as_int(a) != 200) ok2 = 0;
    if (as_int(v[0] + 100) != 212) ok2 = 0;
    if (as_long(a) != 200) ok2 = 0;
    if (two(1, a) != 200) ok2 = 0;
    if (two(1, get(0)) != 112) ok2 = 0;

    if (as_int(sc) != -5) ok3 = 0;
    if (as_long(sc) != -5) ok3 = 0;
    if (as_long(-1) != -1) ok3 = 0;
    if (as_long(~0) != -1) ok3 = 0;

    OUT[0] = ok0; OUT[1] = ok1; OUT[2] = ok2; OUT[3] = ok3; OUT[4] = 0xAA;
    return 0;
}
