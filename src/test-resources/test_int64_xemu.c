/* test_int64_xemu.c — int64_t / uint64_t of <stdint.h> (the __int(64) wide
 * integer struct, its header added by cc45)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] int64_t from integer constants and variables (initializer, =)
 *   [1] + - * with int64_t and integer operands
 *   [2] int64_t narrowed to long/int/char (assignment, cast, return)
 *   [3] int64_t parameters from integers; through pointers
 *   [4] comparisons (== != <)
 *   [5] 64-bit LL constants (hex, beyond 32 bits): initializer, ==
 *   [6] c ? LL : LL keeps all 64 bits (initializer, comparison)
 *   [7] $AA sentinel
 */
#include <stdint.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static int64_t g64;
static int32_t low32(int64_t v) { return v; }
static int64_t twice(int64_t v) { return v + v; }

int main(void) {
    int64_t a = 100000, b = 3, c;
    int32_t l;
    int i;
    signed long big = 70000L;
    unsigned char *pb;

    c = big;
    pb = (unsigned char *)&c;
    OUT[0] = pb[0] == 0x70 && pb[1] == 0x11 && pb[2] == 0x01 && pb[3] == 0 && pb[7] == 0;

    c = a * b - 50000;
    l = c;
    OUT[1] = l == 250000L && (int32_t)(a + 1) == 100001L;

    i = (int)b;
    l = low32(a);
    OUT[2] = i == 3 && l == 100000L && (char)(a + 155) == (char)100155L;

    g64 = 5;
    *(&g64) = twice(g64);
    l = g64;
    OUT[3] = l == 10 && low32(7) == 7;

    OUT[4] = (a == 100000) && (b != a) && (b < a);
    c = 0xffffffff00000001LL;
    pb = (unsigned char *)&c;
    OUT[5] = pb[0] == 1 && pb[3] == 0 && pb[4] == 0xFF && pb[7] == 0xFF && c == 0xffffffff00000001LL && c != 1;

    c = i == 3 ? 0x1ffffffffLL : 0x123456789LL;
    pb = (unsigned char *)&c;
    OUT[6] = pb[0] == 0xFF && pb[4] == 1 && (a != (i ? 0x1ffffffffLL : 0x186a0LL));
    OUT[7] = 0xAA;
    return 0;
}
