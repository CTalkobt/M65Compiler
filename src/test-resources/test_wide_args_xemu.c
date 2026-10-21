/* test_wide_args_xemu.c — arguments converted to the parameter's type
 * (found with pr93213 once memcpy no longer read address 0 as zeros)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] an integer passed to a wide integer (__int128) parameter of a static
 *       function returning one (its hidden result pointer comes first): the
 *       integer's value, not 16 bytes copied from that address
 *   [1] u128 -= an unsigned char variable: the variable's value (not the low
 *       byte of its address)
 *   [2] -1 passed to an unsigned long parameter is all ones (as assigned)
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned __int128 u128;

static u128 twice(unsigned char k, u128 v) { u128 t = v + v; return t + k; }
static unsigned char top(unsigned long w) { return (unsigned char)(w >> 24); }

unsigned char one = 1;

int main(void) {
    u128 r = twice(1, 5);
    u128 a = 0;
    unsigned char ok = 1, i;
    unsigned long d = -1;
    for (i = 1; i < 16; i++) if (((unsigned char *)&r)[i]) ok = 0;
    OUT[0] = ok && ((unsigned char *)&r)[0] == 11;
    a -= one;
    ok = 1;
    for (i = 0; i < 16; i++) if (((unsigned char *)&a)[i] != 0xFF) ok = 0;
    OUT[1] = ok;
    OUT[2] = top(-1) == 0xFF && top(d) == 0xFF;
    OUT[3] = 0xAA;
    return 0;
}
