/* test_bit_type_xemu.c — the __bit type (#73): booleans packed 8 to a byte
 *
 * Results at $C0C0 (01 = pass):
 *   [0] initial values (= 1 / none), packed: 11 __bit globals in 2 bytes
 *   [1] assignment converts to 0 / 1 (512, an int that is 0, a pointer)
 *   [2] a write changes only its own bit (the neighbours kept)
 *   [3] !, &&, ||, op= (flag |= 4 is 1), in a loop condition
 *   [4] __zp __bit: in the base page (smb / rmb / bbs), the same results
 *   [5] struct members: 1-bit fields, 3 in one byte
 *   [6] locals and parameters: _Bool (one byte each), a return value
 *   [7] sizeof a __bit global is 1 (a _Bool's)
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

__bit b0 = 1, b1, b2 = 1, b3, b4, b5, b6, b7 = 1;
static __bit b8, b9 = 1, b10;
__zp __bit z0, z1 = 1, z2;

struct Flags { __bit visible; __bit dirty; __bit locked; unsigned char after; } fl = { 1, 0, 1, 0x5A };

static __bit not_bit(__bit v) { return !v; }

static unsigned char count_set(void) {
    return (unsigned char)(b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8 + b9 + b10);
}

int main(void) {
    unsigned char ok, n;
    unsigned int zero = 0, big = 512;
    unsigned char *ptr = (unsigned char *)0x100;

    OUT[0] = b0 && !b1 && b2 && b7 && !b8 && b9 && !b10 && count_set() == 4;

    b1 = big;
    b3 = zero;
    b4 = ptr;
    OUT[1] = b1 == 1 && b3 == 0 && b4 == 1 && count_set() == 6;

    ok = 1;
    b5 = 1;
    if (!b0 || !b1 || !b2 || b3 || !b4 || !b5 || b6 || !b7) ok = 0;
    b5 = 0;
    if (!b4 || b6) ok = 0;
    b10 = 1;
    if (!b9 || !b10 || b8) ok = 0;
    OUT[2] = ok && count_set() == 7;

    ok = 1;
    b6 = !b6;                 /* 1 */
    if (!(b6 && b0) || (b3 || b8)) ok = 0;
    b3 |= 4;                  /* 1 */
    b8 ^= 1;                  /* 1 */
    b8 &= 0;                  /* 0 */
    if (b3 != 1 || b8 != 0) ok = 0;
    n = 0;
    b8 = 1;
    while (b8) { n++; if (n == 3) b8 = 0; }
    OUT[3] = ok && n == 3;

    ok = !z0 && z1 && !z2;
    z0 = 1; z1 = 0; z2 = big;
    if (!z0 || z1 || !z2) ok = 0;
    z2 = 0;
    if (z2) ok = 0;
    n = 0;
    if (z0) n++;
    if (!z1) n++;
    OUT[4] = ok && n == 2;

    ok = sizeof(struct Flags) == 2 && fl.visible && !fl.dirty && fl.locked && fl.after == 0x5A;
    fl.dirty = 7;
    fl.visible = 0;
    OUT[5] = ok && fl.dirty == 1 && !fl.visible && fl.locked && fl.after == 0x5A;

    {
        __bit l = big;
        OUT[6] = l == 1 && sizeof(l) == 1 && not_bit(0) == 1 && not_bit(300) == 0;
    }
    OUT[7] = sizeof(b0) == 1;
    OUT[8] = 0xAA;
    return 0;
}
