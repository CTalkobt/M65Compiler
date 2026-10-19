/* test_arg_target_xemu.c — a call's arguments computed in their parameter
 * slots ($10-$2F, ZP convention), not in a temporary copied there: only
 * when nothing between the computation and the call uses the slots
 *
 * Results at $C0C0 (01 = pass):
 *   [0] arguments computed just before the call (int, char, long, pointer)
 *   [1] an argument computed before another one that calls a function
 *       (f(a + 1, g(b))): the call in between would overwrite the slot
 *   [2] nested calls as arguments: f(g(x) + 1, h(y) * 2)
 *   [3] a division and a float conversion between an argument and its call
 *   [4] consecutive calls with arguments from the same values; __regparm
 *   [5] a long and a char argument after an int one (slot offsets)
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

NOINLINE static unsigned int sub3(unsigned int a, unsigned int b, unsigned int c) { return a - b - c; }
NOINLINE static unsigned int twice(unsigned int x) { return x * 2; }
NOINLINE static unsigned int pair(unsigned int a, unsigned int b) { return a * 100 + b; }
NOINLINE static unsigned long mix(unsigned int a, unsigned long b, unsigned char c) { return b - a + c; }
NOINLINE static unsigned char low(char *p, unsigned char k) { return (unsigned char)p[k]; }
NOINLINE __regparm static unsigned int rp(unsigned int a, unsigned int b) { return a - b; }

static char text[] = "abcdef";
volatile unsigned int vone = 1;

int main(void) {
    unsigned int a = 40, b = 7, r;
    unsigned long l = 0x10000UL;
    float f = 2.5f;

    OUT[0] = sub3(a + 10, b * 2, a - b) == 50 - 14 - 33 && low(text + 2, (unsigned char)(b - 5)) == 'e' &&
             mix(a + 1, l + 5, (unsigned char)(b + 1)) == 0x10000UL + 5 - 41 + 8;
    OUT[1] = pair(a + 1, twice(b)) == 4114 && pair(twice(a), b + 1) == 8008;
    OUT[2] = pair(twice(b) + 1, twice(a) * 2) == 1500 + 160 && sub3(twice(1), twice(2), twice(vone)) == (unsigned int)(2 - 4 - 2);
    r = pair(a / b, (unsigned int)(f * 4.0f));
    OUT[3] = r == 510 && pair(a % b + 1, (unsigned int)f) == 602;
    r = pair(a + 1, b) + pair(a + 1, b) + rp(a + 2, b + 2);
    OUT[4] = r == 2 * 4107 + 33;
    OUT[5] = mix(b * 3, l * 2 + 1, (unsigned char)(a + 3)) == 0x20000UL + 1 - 21 + 43;
    OUT[6] = 0xAA;
    return 0;
}
