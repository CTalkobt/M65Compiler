/* test_char_promotion_xemu.c — chars in int arithmetic and as int arguments
 *
 * + - * << of chars are ints: a char operand added to such a sum must not
 * cut it back to 8 bits (a + (b + c) lost the carry of b + c, 600 -> 344).
 * A narrower argument widens to its parameter by its own signedness: an
 * unsigned char 200 passed to a signed int is 200 (with -fsigned-default
 * it was sign-extended to -56); a signed char stays negative; a constant
 * keeps the parameter's signedness (-1 to a signed long is -1). A signed
 * int or long against an unsigned char is signed (the char's values all fit
 * in it): -3 < 245, -6 / 3 is -2 (they were compared and divided unsigned).
 * A signed and an unsigned char both promote to int: -1 < 255 (it was an
 * 8-bit compare of the same byte), and so /, % and /=. The right side of a
 * compound assignment widens by its own signedness (l += (signed char)-1).
 * A char difference or negation whose only use is a char is computed in 8
 * bits on the bytes themselves; used as an int it keeps 16 bits.
 * Run in both conventions, with and without -fsigned-default.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sums of three and four chars, nested and from calls
 *   [1] - * << of chars inside a wider sum
 *   [2] unsigned char arguments to signed int / signed long parameters
 *   [3] signed char and constant arguments to wider signed parameters
 *   [4] signed int / long against an unsigned char: compares, / % /=;
 *       a signed long /= an unsigned int
 *   [5] signed char against unsigned char: compares, / % /= %=; a signed
 *       char on the right of += and /= on wider types
 *   [6] char differences and negations stored as chars (8 bits, wrapping)
 *       and used as ints (not narrowed: -20)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

volatile unsigned char a = 200, b = 200, c = 200, d = 112;
volatile signed char sc = -5;
volatile signed int sneg = -3, s6 = -6;
volatile signed long lneg = -3;
volatile unsigned char c245 = 245, c3 = 3;
volatile unsigned int u65533 = 65533, u3 = 3;
volatile signed long l6 = -6;
volatile signed char scm1 = -1, scm3 = -3, scm6 = -6;
volatile unsigned char uc255 = 255, uc1 = 1;
volatile signed int s600 = 600;
volatile unsigned char ua10 = 10, ub30 = 30, uc7 = 7;
volatile signed char sa10 = 10, sb30 = 30, sm5 = -5;
static unsigned char ucs[3];
static unsigned char v[4] = {112, 112, 112, 112};

N static unsigned char get(unsigned char i) { return v[i]; }
N static signed int as_int(signed int x) { return x; }
N static signed long as_long(signed long x) { return x; }
N static signed int two(unsigned char k, signed int x) { return x + k - k; }

int main(void) {
    unsigned int s;
    unsigned char ok0 = 1, ok1 = 1, ok2 = 1, ok3 = 1, ok4 = 1, ok5 = 1, ok6 = 1;
    unsigned char ux, uy;
    signed char sx;
    signed int di;
    signed char xa, xb;
    unsigned char xu;
    signed int si, sq;
    signed long lq;

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

    si = sneg; sq = s6;
    if (si >= c245) ok4 = 0;
    if (!(c245 > si)) ok4 = 0;
    if (!(si < c245)) ok4 = 0;
    if (si == c245) ok4 = 0;
    if (lneg >= c245) ok4 = 0;
    if (sq / c3 != -2) ok4 = 0;
    if (sq % c3 != 0) ok4 = 0;
    sq /= c3;                               if (sq != -2) ok4 = 0;
    if (!(u65533 > c245)) ok4 = 0;          /* unsigned int: unsigned */
    lq = l6; lq /= u3;                      if (lq != -2) ok4 = 0;

    xa = scm1; xu = uc255;
    if (!(xa < xu)) ok5 = 0;
    if (!(xu > xa)) ok5 = 0;
    if (xa == xu) ok5 = 0;
    xu = uc1;
    if (!(xa < xu)) ok5 = 0;
    xa = scm6; xb = scm6; xu = c3;
    if (xa / xu != -2) ok5 = 0;
    if (xa % xu != 0) ok5 = 0;
    xa /= xu;                               if (xa != -2) ok5 = 0;
    xb %= xu;                               if (xb != 0) ok5 = 0;
    lq = 1000; lq += scm1;                  if (lq != 999) ok5 = 0;
    si = s600; si /= scm3;                  if (si != -200) ok5 = 0;

    ux = ua10; uy = ub30;
    xu = ux - uy;                           if (xu != 236) ok6 = 0;
    xu = -ux;                               if (xu != 246) ok6 = 0;
    xu = ~ux;                               if (xu != 245) ok6 = 0;
    xu = ux - uy - uc7;                     if (xu != 229) ok6 = 0;
    sx = sa10 - sb30;                       if (sx != -20) ok6 = 0;
    sx = -sm5;                              if (sx != 5) ok6 = 0;
    di = (signed int)ux - uy;               if (di != -20) ok6 = 0;
    di = sa10 - sb30;                       if (di != -20) ok6 = 0;
    if ((unsigned char)(ux - uy) != 236) ok6 = 0;
    ucs[0] = ux; ucs[1] = uy;
    ucs[2] = ucs[0] - ucs[1];               if (ucs[2] != 236) ok6 = 0;

    OUT[0] = ok0; OUT[1] = ok1; OUT[2] = ok2; OUT[3] = ok3; OUT[4] = ok4; OUT[5] = ok5; OUT[6] = ok6;
    OUT[7] = 0xAA;
    return 0;
}
