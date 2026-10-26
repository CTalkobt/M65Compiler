/* test_fixed_any_xemu.c — __fixed(I, F) (#72): fixed-point types of any
 * split of 16 or 32 bits (the expected integers from Python: round(v * 2^F);
 * products and quotients truncated toward 0)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __fixed(4, 28): constants, *, / (the library at F = 28), 0.1 * 0.1,
 *       1 / 3
 *   [1] __fixed(8, 24): 100.25 * 0.5, 100.25 / 4
 *   [2] __fixed(1, 15) (audio): 0.5 * -0.5, 0.75, comparisons
 *   [3] __fixed(4, 12): 3.25 / 0.5, 3.25 * 1.5 (inline, 16 bits)
 *   [4] mixed: __fixed(4, 12) with __fixed(4, 28) (the wider), with int,
 *       with float, a cast between two of one width
 *   [5] conversions: to int toward 0, to float, fixedToFloat / fixedToInt,
 *       a global array and a function of __fixed(4, 28)
 *   [6] fixed * int and fixed / int: the integer times / over the fixed
 *       value's integer (-160 * 0.01 in __fixed(4, 28), where -160 itself
 *       does not fit)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define RAW16(x) (*(unsigned int *)&(x))
#define RAW32(x) (*(unsigned long *)&(x))

typedef __fixed(4, 28) f4_28;

f4_28 table[3] = { 0.5, -1.25, 7.5 };

static f4_28 twice(f4_28 v) { return v * 2; }

int main(void) {
    unsigned char ok;
    f4_28 a = 1.5, b = -0.75, c, t = 0.1;
    __fixed(8, 24) h = 100.25, r;
    __fixed(1, 15) s = 0.5, u;
    __fixed(4, 12) p = 3.25, q;
    signed int i;
    float f;

    ok = RAW32(a) == 0x18000000UL && RAW32(b) == 0xF4000000UL && RAW32(t) == 0x0199999AUL;
    c = a * b;                  if (RAW32(c) != 0xEE000000UL) ok = 0;
    c = a / b;                  if (RAW32(c) != 0xE0000000UL) ok = 0;
    c = t * t;                  if (RAW32(c) != 0x0028F5C2UL) ok = 0;
    c = (f4_28)1 / 3;           if (RAW32(c) != 0x05555555UL) ok = 0;
    OUT[0] = ok;

    r = h * 0.5;
    ok = RAW32(r) == 0x32200000UL;
    r = h / 4;                  if (RAW32(r) != 0x19100000UL) ok = 0;
    OUT[1] = ok;

    u = s * -0.5;
    ok = RAW16(u) == 0xE000;
    u = 0.75;                   if (RAW16(u) != 0x6000) ok = 0;
    if (!(u > s) || s < 0 || u == 0.5) ok = 0;
    OUT[2] = ok;

    q = p / 0.5;
    ok = RAW16(q) == 0x6800;
    q = p * 1.5;                if (RAW16(q) != 0x4E00) ok = 0;
    OUT[3] = ok;

    c = p + a;                  /* 3.25 + 1.5 in __fixed(4, 28) */
    ok = RAW32(c) == 0x4C000000UL;
    i = 2;
    c = a + i;                  if (c != 3.5) ok = 0;
    f = 0.25f;
    c = a - f;                  if (c != 1.25) ok = 0;
    r = (__fixed(8, 24))a * 2;  /* a cast between two of 32 bits */
    if (r != 3) ok = 0;
    OUT[4] = ok;

    ok = 1;
    c = -3.75;
    i = c;                      if (i != -3) ok = 0;
    f = c;                      if (f != -3.75f) ok = 0;
    if (fixedToInt(b) != 0 || fixedToFloat(a) != 1.5f) ok = 0;
    if (table[0] != 0.5 || table[1] != -1.25 || table[2] != 7.5) ok = 0;
    if (twice(table[1]) != -2.5) ok = 0;
    OUT[5] = ok;
    {
        f4_28 st = 0.01;
        signed int n = -160;
        c = n * st;             if (RAW32(c) != 0xE6666620UL) ok = 0;   /* -1.6: the integers exactly */
        c = st * 7;             if (RAW32(c) != 0x011EB855UL) ok = 0;
        c = st / 3;             if (RAW32(c) != 0x000DA741UL) ok = 0;
        OUT[6] = ok;
    }
    OUT[7] = 0xAA;
    return 0;
}
