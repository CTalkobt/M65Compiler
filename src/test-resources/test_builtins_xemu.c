/* test_builtins_xemu.c — GCC builtins and printf formatting
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __builtin_ffs/clz/ctz/popcount/parity (int and long), bswap16/32
 *   [1] __builtin_offsetof (nested member, array element) as a constant
 *   [2] __builtin_expect evaluates both operands; __builtin_choose_expr and
 *       __builtin_constant_p
 *   [3] __builtin_return_address(0) is the same for calls from one site
 *   [4] __builtin_{add,sub,mul}_overflow (generic, typed and _p forms)
 *   [5] sprintf flags, width, precision and length modifiers
 *   [6] ~ and unary - on long; a long tested for truth in &&
 *   [7] $AA sentinel
 */
#include <stdio.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

struct In { char c; int v[3]; };
struct Out { int a; struct In in; };
static const int off = __builtin_offsetof(struct Out, in.v[2]);

static int side;
static int bump(void) { return ++side; }

static void *ra(void) __attribute__((noinline));
static void *ra(void) { return __builtin_return_address(0); }
/* one call site for ra(): not inlined into its callers */
static void *call_ra(void) __attribute__((noinline));
static void *call_ra(void) { return ra(); }

static int fmt_ok(const char *want, const char *fmt, long v) {
    char buf[32];
    int n = sprintf(buf, fmt, v);
    return n == (int)strlen(want) && strcmp(buf, want) == 0;
}

int main(void) {
    int ok;
    volatile long lv = 0x00F00000L;
    volatile int iv = 0x0018;

    ok = __builtin_ffs(iv) == 4 && __builtin_ffsl(lv) == 21 && __builtin_ffs(0) == 0 &&
         __builtin_clz(iv) == 11 && __builtin_clzl(lv) == 8 &&
         __builtin_ctz(iv) == 3 && __builtin_ctzl(lv) == 20 &&
         __builtin_popcount(iv) == 2 && __builtin_popcountl(lv) == 4 &&
         __builtin_parity(iv) == 0 && __builtin_parityl(lv | 1) == 1 &&
         __builtin_bswap16(0x1234) == 0x3412 && __builtin_bswap32(0x12345678UL) == 0x78563412UL;
    OUT[0] = ok;

    OUT[1] = off == 2 + 1 + 4;

    side = 0;
    ok = __builtin_expect(bump(), 1) == 1 && side == 1;
    ok = ok && __builtin_choose_expr(sizeof(int) == 2, 10, side++) == 10 && side == 1;
    ok = ok && __builtin_constant_p(3 + 4) && !__builtin_constant_p(side);
    OUT[2] = ok;

    {
        void *a = call_ra(), *b = call_ra();
        OUT[3] = a == b && a != 0 && ra() != a;
    }

    {
        signed int r; unsigned char uc; signed char sc; unsigned long ul;
        volatile unsigned int two = 2;
        ok = !__builtin_mul_overflow(two, (signed int)-16384, &r) && r == -32768;
        ok = ok && __builtin_mul_overflow(two, (signed int)16384, &r);
        ok = ok && __builtin_add_overflow(200, 100, &uc) && uc == 44;
        ok = ok && !__builtin_sub_overflow(-100, 28, &sc) && sc == -128;
        ok = ok && __builtin_sub_overflow(1, 2, &ul) && ul == 0xFFFFFFFFUL;
        ok = ok && __builtin_add_overflow_p(32767, 1, (signed int)0);
        ok = ok && !__builtin_add_overflow_p(32767, 1, (unsigned int)0);
        ok = ok && __builtin_sadd_overflow(32767, 1, &r) && r == -32768;
        ok = ok && !__builtin_umull_overflow(65536UL, 65535UL, &ul) && ul == 0xFFFF0000UL;
        OUT[4] = ok;
    }

    ok = fmt_ok("0177", "%#hho", 127) && fmt_ok("0x7f", "%#hhx", 127) && fmt_ok("0", "%#x", 0) &&
         fmt_ok("  -42", "%5d", -42) && fmt_ok("-0042", "%05d", -42) && fmt_ok("42   |", "%-5d|", 42) &&
         fmt_ok("+7", "%+d", 7) && fmt_ok(" 7", "% d", 7) && fmt_ok("007", "%.3d", 7) &&
         fmt_ok("-123456", "%ld", -123456L) && fmt_ok("FFFF", "%X", -1) && fmt_ok("-1", "%hhd", 255) &&
         fmt_ok("ab", "%.2s", (long)(unsigned)"abc");
    {
        char buf[8];
        int n = snprintf(buf, 4, "%d", 12345);
        ok = ok && n == 5 && strcmp(buf, "123") == 0;
    }
    OUT[5] = ok;

    {
        volatile unsigned long x = 0x0000FFFFUL;
        volatile long y = 0x10000L;
        volatile unsigned long hi = 0x80000000UL;
        ok = ~x == 0xFFFF0000UL && -y == -65536L && (~x + 1) == 0xFFFF0001UL;
        ok = ok && (iv && (hi & 0x80000000UL)) && !(iv && (hi & 0x7FFFFFFFUL));
        OUT[6] = ok;
    }

    OUT[7] = 0xAA;
    return 0;
}
