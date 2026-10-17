/* test_constprop_xemu.c — constants through variables (IR optimizer)
 *
 * A local or parameter assigned a constant once, before any read, is that
 * constant to the optimizer (one = 1; one * x). Where it must not be:
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the plain cases: a local, a parameter overwritten first, a loop
 *       using the constant
 *   [1] a parameter read before it is overwritten keeps its incoming value
 *   [2] a variable assigned in a branch or twice is not a constant
 *   [3] a variable whose address is taken (changed through the pointer)
 *   [4] a variable a nested function changes
 *   [5] a variable assigned in a loop (the store is not in the entry block)
 *   [6] a variable changed by inline assembly (a register variable)
 *   [7] a local or parameter that shadows a const global is its own,
 *       writable variable (Issue #285); the global keeps its value
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

NOINLINE static unsigned plain(unsigned x) {
    unsigned one = 1, zero = 0, four = 4;
    return one * x + (zero ^ x) + four * x;          /* 6x */
}

NOINLINE static unsigned param_first(unsigned x, unsigned k) {
    k = 3;
    return k * x;
}

NOINLINE static unsigned loop_const(unsigned n) {
    unsigned step = 8, i, s = 0;
    for (i = 0; i < n; i++) s += step * i;
    return s;
}

NOINLINE static unsigned param_read_first(unsigned x, unsigned k) {
    unsigned before = k * x;      /* the incoming k */
    k = 2;
    return before + k * x;
}

NOINLINE static unsigned branch_assign(unsigned x, unsigned c) {
    unsigned m = 1;
    if (c) m = 5;
    return m * x;
}

NOINLINE static unsigned twice(unsigned x) {
    unsigned m = 2;
    x = m * x;
    m = 3;
    return m * x;
}

NOINLINE static void set7(unsigned *p) { *p = 7; }

NOINLINE static unsigned addr_taken(unsigned x) {
    unsigned m = 1;
    set7(&m);
    return m * x;
}

NOINLINE static unsigned nested(unsigned x) {
    unsigned m = 1;
    void bump(void) { m = 9; }
    bump();
    return m * x;
}

NOINLINE static unsigned in_loop(unsigned x) {
    unsigned i, m, s = 0;
    for (i = 0; i < 3; i++) {
        m = i + 1;
        s += m * x;
    }
    return s;
}

NOINLINE static unsigned char asm_changed(void) {
    register unsigned char r = 1;
    __asm__("lda #$05\n sta @_l_r");
    return r * 3;
}

const unsigned char cr = 13;

NOINLINE static unsigned char shadow_local(unsigned char c) {
    unsigned char cr;
    cr = c;
    cr++;
    return cr;
}

NOINLINE static unsigned char shadow_param(unsigned char cr) {
    cr = cr + 1;
    return cr;
}

NOINLINE static unsigned char shadow_block(unsigned char c) {
    unsigned char s = 0;
    {
        unsigned char cr;
        for (cr = 0; cr < c; cr++) s += 2;
    }
    return s + cr;                /* the global again */
}

int main(void) {
    OUT[0] = plain(5) == 30 && param_first(4, 100) == 12 && loop_const(4) == 8 * 6;
    OUT[1] = param_read_first(3, 10) == 30 + 6;
    OUT[2] = branch_assign(4, 0) == 4 && branch_assign(4, 1) == 20 && twice(5) == 30;
    OUT[3] = addr_taken(3) == 21;
    OUT[4] = nested(2) == 18;
    OUT[5] = in_loop(10) == 60;
    OUT[6] = asm_changed() == 15;
    OUT[7] = shadow_local(40) == 41 && shadow_param(7) == 8 && shadow_block(3) == 6 + 13 && cr == 13;
    OUT[8] = 0xAA;
    return 0;
}
