/* main.c — the resident part of the overlay test (test_overlay_xemu.sh)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] calls into overlay a, a call inside it
 *   [1] a into b, b into resident code, back to a (a copied in again)
 *   [2] function pointers to overlay functions: from resident code, from
 *       overlay a to b and to resident code; a table of them in data
 *   [3] recursion bouncing between the overlays (20 levels)
 *   [4] a long argument and result; static data and BSS of the overlays
 *       kept across swaps
 *   [5] disk I/O from inside an overlay (a file written and read back)
 *   [6] $AA sentinel
 */
#include "ovl.h"

#define OUT ((volatile unsigned char *)0xC0C0)

int res_calls;
int res_inc(int x) { res_calls++; return x + 1; }

static int res_double(int x) { return x * 2; }

static int (*const table[3])(int) = { b_neg, a_twice, res_double };

int main(void) {
    int (*f)(int) = b_neg;
    int i, ok;
    OUT[0] = a_add(2, 3) == 5 && a_twice(21) == 42;
    OUT[1] = a_via_b(5) == 15 + 2 && res_calls == 1;
    ok = f(7) == -7 && a_apply(b_neg, 4) == -3 && a_apply(res_double, 4) == 9 && a_apply(a_twice, 3) == 7;
    for (i = 0; i < 3; i++) ok = ok && table[i](5) == (i == 0 ? -5 : 10);
    OUT[2] = ok;
    OUT[3] = a_rec(20) == 10 * 11 && b_rec(3) == 10 + 1 + 10;
    OUT[4] = a_long(100000L) == 370000L && a_count() == 8 * 1000 + 113 && b_count() == 4;
    OUT[5] = b_file();
    OUT[6] = 0xAA;
    return 0;
}
