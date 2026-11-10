/* test_licm_rotate_xemu.c — a value hoisted out of a rotated loop stays live
 * across the loop's calls (#464)
 *
 * LICM hoists py[0]'s address (a copy of py) into the loop's pre-header; the
 * loop reads it at the top of its body, before the calls. Rotated (the test
 * after the latch, -frotate-loops), the latch is an empty block the back edge
 * goes to: the register allocator's loop detection skipped such a loop, the
 * value's live range ended at its first read, and it went into a register of
 * the pool the calls overwrite (basic_gfx.c's facet: wrong columns).
 * Each check writes its number at $C0C0 + i; $AA after the last.
 *
 * Expected at $C0C0: 01 02 03 AA
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC0C0 + (i)))

volatile int sink;

/* both use the register pool ($30 on) for their temporaries; not inlined */
__attribute__((noinline)) static int mix(int a, unsigned int b) {
    long t = (long)a * 3 + (long)(b * 5u);
    long u = (long)(a ^ (int)b) - (t >> 2);
    return (int)(t - u);
}

__attribute__((noinline)) static void other(unsigned int b) {
    long k = (long)b * 7 + 1;
    sink = (int)(k ^ (k << 3));
}

/* sum of mix(py[0], fx) for fx from px[0] to px[1] */
__attribute__((noinline)) static long walk(int *px, int *py) {
    unsigned int fx = (unsigned int)px[0];
    long sum = 0;
    for (;;) {
        unsigned char last;
        sum += mix(py[0], fx);
        other(fx);
        last = fx >= (unsigned int)px[1];
        fx++;
        if (last) break;
    }
    return sum;
}

static long expect(int y, int x0, int x1) {
    long s = 0;
    int x;
    for (x = x0; x <= x1; x++) {
        long t = (long)y * 3 + (long)((unsigned int)x * 5u);
        long u = (long)(y ^ x) - (t >> 2);
        s += (int)(t - u);
    }
    return s;
}

int px[2] = {3, 40}, py[2] = {11, 0};

int main(void) {
    int i;
    for (i = 0; i < 4; i++) RESULT(i) = 0;
    if (walk(px, py) == expect(11, 3, 40)) RESULT(0) = 0x01;
    px[0] = 100; px[1] = 101; py[0] = -7;
    if (walk(px, py) == expect(-7, 100, 101)) RESULT(1) = 0x02;
    px[0] = 5; px[1] = 5; py[0] = 2;
    if (walk(px, py) == expect(2, 5, 5)) RESULT(2) = 0x03;
    RESULT(3) = 0xAA;
    return 0;
}
