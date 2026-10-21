/* test_load_reuse_xemu.c — CSE reuses a load of a global across stores to
 * other globals (#320), and loads again whenever the value may have changed
 *
 * Results at $C0C0 (01 = pass):
 *   [0] x[i] read, stores to other arrays, x[i] read again: the same value
 *   [1] a store into the same array (another index that is the same one at
 *       run time) between the reads: the new value
 *   [2] a store through a pointer (into x) between the reads
 *   [3] a store through a local pointer to x, and a call that changes x
 *   [4] across ?: diamonds: stores to other arrays in the arms (reused), a
 *       store to x in one arm (read again)
 *   [5] a global struct: a member stored between reads of another, then of
 *       the same one
 *   [6] a volatile global is read each time (a change by asm between the
 *       reads; tick_twice's two reads are checked in its assembly)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int x[4] = { 100, 200, 300, 400 };
unsigned int y[4];
unsigned char lo[4], hi[4];
unsigned int *px;
struct pair { unsigned int a, b; } s = { 1, 2 };
volatile unsigned char tick;

void bump_x(unsigned char i) { x[i] += 1000; }

/* two reads of a volatile global with nothing between: both stay
   (test_xemu.sh checks the assembly reads _tick twice here) */
unsigned char tick_twice(void) { return tick + tick; }

int main(void) {
    volatile unsigned char vi = 1, vj = 1, v0 = 0;
    unsigned char i = vi, j = vj;
    unsigned int a, b, c;

    a = x[i];
    lo[i] = (unsigned char)a;
    hi[i] = (unsigned char)(a >> 8);
    y[i] = a + 5;
    b = x[i];
    OUT[0] = a == 200 && b == 200 && y[1] == 205 && lo[1] == 200 && hi[1] == 0;

    a = x[i];
    x[j] = 77;
    b = x[i];
    OUT[1] = a == 200 && b == 77;

    px = &x[2];
    a = x[2];
    *px = 9;
    b = x[2];
    OUT[2] = a == 300 && b == 9;

    {
        unsigned int *q = x;
        a = x[3];
        q[3] = 33;
        b = x[3];
        bump_x(3);
        c = x[3];
        OUT[3] = a == 400 && b == 33 && c == 1033;
    }

    x[0] = 0x0180;
    a = (x[i - 1] & 0x100) ? 1 : 0;
    lo[0] = (unsigned char)x[i - 1];
    b = (x[i - 1] & 0x80) ? 2 : 0;
    hi[0] = v0 ? 1 : (unsigned char)(x[i - 1] >> 8);
    if (v0) x[0] = 5; else x[0] = 6;
    c = x[i - 1];
    OUT[4] = a == 1 && b == 2 && lo[0] == 0x80 && hi[0] == 1 && c == 6;

    a = s.a;
    s.b = 20;
    b = s.a;
    s.a = 10;
    c = s.a;
    OUT[5] = a == 1 && b == 1 && c == 10 && s.b == 20;

    tick = 0;
    a = tick;
    __asm__("inc _tick");
    b = tick;
    OUT[6] = a == 0 && b == 1 && tick_twice() == 2;
    OUT[7] = 0xAA;
    return 0;
}
