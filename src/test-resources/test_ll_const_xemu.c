/* test_ll_const_xemu.c — long long results and constants (#338's
 * relatives): an integer returned from a long long function is converted
 * (sign-extended when signed), and constant expressions with an LL operand
 * are folded in 64 bits
 *
 * Results at $C0C0 (01 = pass):
 *   [0] return of a signed int: -1, ~a, -a, a variable
 *   [1] return of unsigned and other widths: 0xFFFF zero-extended, an
 *       unsigned long, a signed char, a literal, a big LL constant; a small
 *       function inlined at -O2
 *   [2] folding: LL & all ones, -4102790424LL, 1LL << 40, (1LL << 40) /
 *       1024, ULL >> 60 (logical), -1LL >> 60 (arithmetic), a comparison
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

__attribute__((noinline)) static long long r_neg1(signed int a) { return a; }
__attribute__((noinline)) static long long r_not(signed int a) { return ~a; }
__attribute__((noinline)) static long long r_minus(signed int a) { return -a; }
__attribute__((noinline)) static long long r_var(signed int a) { signed int b = a * 2; return b; }
__attribute__((noinline)) static long long r_u(unsigned int u) { return u; }
__attribute__((noinline)) static long long r_ul(unsigned long u) { return u; }
__attribute__((noinline)) static long long r_sc(signed char c) { return c; }
__attribute__((noinline)) static long long r_lit(void) { return 5; }
__attribute__((noinline)) static long long r_big(void) { return 4102790424LL; }
static long long r_inl(signed int a) { return a - 1; }

long long g;

int main(void) {
    unsigned char ok;

    ok = r_neg1(-1) == -1LL && r_not(0) == -1LL && r_minus(1) == -1LL && r_var(-3) == -6LL;
    if (r_neg1(-1) >= 0 || r_neg1(5) != 5) ok = 0;
    OUT[0] = ok;

    ok = r_u(0xFFFF) == 65535LL && r_ul(0xFFFFFFFFUL) == 4294967295LL && r_sc(-2) == -2LL && r_lit() == 5LL &&
         r_big() == 4102790424LL && r_big() > 0 && r_inl(0) == -1LL;
    OUT[1] = ok;

    ok = 1;
    g = 4102790424LL & 0xFFFFFFFFFFFFFFFFLL;
    if (g != 4102790424LL || g < 0) ok = 0;
    g = -4102790424LL;
    if (g >= 0 || -g != 4102790424LL) ok = 0;
    g = 1LL << 40;
    if (g >> 40 != 1 || g == 0) ok = 0;
    g = (1LL << 40) / 1024;
    if (g != 1073741824LL) ok = 0;
    g = 0xFFFFFFFFFFFFFFFFULL >> 60;
    if (g != 15) ok = 0;
    g = -1LL >> 60;
    if (g != -1) ok = 0;
    if (!(4102790424LL > 0) || (4102790424LL < 1)) ok = 0;
    OUT[2] = ok;
    OUT[3] = 0xAA;
    return 0;
}
