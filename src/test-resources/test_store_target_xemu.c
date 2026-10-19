/* test_store_target_xemu.c — a value computed for a store into a local with a
 * base-page home (a function that makes calls keeps its most used locals
 * there) is computed in that home, not in a temporary copied over: only when
 * nothing between the computation and the store mentions the local
 *
 * Results at $C0C0 (01 = pass):
 *   [0] 8-, 16- and 32-bit locals set from expressions, used across calls
 *   [1] the old value of the local read after the new one is computed but
 *       before the store (x2 = x + 1; y = x; x = x2): must see the old one
 *   [2] x = x + k and x = k - x (the local is an operand)
 *   [3] a loop counting with locals updated each round
 *   [4] a value computed, then a call, then stored
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

volatile unsigned int vin = 3;
NOINLINE static unsigned int id(unsigned int v) { return v; }
NOINLINE static unsigned long lid(unsigned long v) { return v; }

NOINLINE static unsigned char widths(void) {
    unsigned char c = (unsigned char)(vin * 5);
    unsigned int i = vin * 100 + 7;
    unsigned long l = (unsigned long)vin * 70000UL;
    unsigned int j = id(i);
    unsigned long m = lid(l);
    c = (unsigned char)(c + id(1));
    return c == 16 && j == 307 && m == 210000UL && i == 307 && l == 210000UL;
}

NOINLINE static unsigned char old_value(void) {
    unsigned int x = id(10), y, x2;
    x2 = x + 1;
    y = x;
    x = x2;
    return id(x) == 11 && id(y) == 10;
}

NOINLINE static unsigned char self(void) {
    unsigned int x = id(10);
    x = x + 5;
    x = id(x) + 0;
    x = 100 - x;
    return id(x) == 85;
}

NOINLINE static unsigned int loop(unsigned int n) {
    unsigned int i, s = 0, t;
    for (i = 0; i < n; i++) {
        t = s + i * 2;
        s = id(t);
    }
    return s;
}

NOINLINE static unsigned char across(void) {
    unsigned int a = id(4), r;
    r = a * 3 + 1;
    a = id(a + 1);
    return id(r) == 13 && a == 5;
}

int main(void) {
    OUT[0] = widths();
    OUT[1] = old_value();
    OUT[2] = self();
    OUT[3] = loop(10) == 90;
    OUT[4] = across();
    OUT[5] = 0xAA;
    return 0;
}
