/* test_bitfield_store_xemu.c — bitfield stores and signed fields
 *
 * Results at $C0C0 (01 = pass):
 *   [0] full-width 16-bit field in a global: store then read back
 *   [1] signed 1- and 7-bit fields read back negative
 *   [2] field after a 16-bit field (pointer member before it)
 *   [3] fields through a pointer and in a local; neighbours untouched
 *   [4] 20-bit field in a 32-bit unit (bits 16+) and a long field
 *   [5] unnamed ':0' bitfield skipped by positional initializers;
 *       bitfields in local and compound-literal initializers
 *   [6] same member name as a bitfield in another struct isn't a bitfield
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct A { unsigned int e0 : 16; } a;
struct B { signed int s : 1; signed int t : 7; } b;
struct C { void *p; int b : 16; unsigned p9 : 9; } c;
struct D { unsigned char x : 3; unsigned char y : 5; unsigned char z; } d, *pd = &d;
struct W { long lo : 12; long hi : 20; } w;
typedef struct { unsigned M1; unsigned M2 : 1; int : 0; unsigned M3 : 1; } S;
struct Bx { int x : 3; int y; } bx;
struct Nx { int x; int y; } nx;

int main(void) {
    volatile int v = 32;
    a.e0 = v;
    OUT[0] = a.e0 == 32 && ((unsigned char *)&a)[0] == 32;

    b.s = -1; b.t = -5;
    OUT[1] = b.s == -1 && b.t == -5 && ((unsigned char *)&b)[0] == 0xF7;

    c.b = 1000; c.p9 = 8;
    OUT[2] = c.p9 == 8 && c.b == 1000;

    d.z = 0x5A;
    pd->x = 5; pd->y = 17;
    {
        struct D l;
        l.z = 9; l.x = 6; l.y = 30;
        OUT[3] = pd->x == 5 && pd->y == 17 && d.z == 0x5A && l.x == 6 && l.y == 30 && l.z == 9;
    }

    w.lo = 0x123; w.hi = 0x54321;
    OUT[4] = w.lo == 0x123 && w.hi == 0x54321;

    {
        S r = { 0, 0, 1 };
        volatile unsigned one = 1;
        S q = { 7, one, one };
        struct D *cl = &(struct D){ .y = 3, .x = 2 };
        OUT[5] = r.M2 == 0 && r.M3 == 1 && q.M1 == 7 && q.M2 == 1 && q.M3 == 1 && cl->x == 2 && cl->y == 3;
    }

    bx.y = 5; nx.x = 1000; nx.y = 77;
    OUT[6] = nx.x == 1000 && nx.y == 77 && bx.y == 5;
    OUT[7] = 0xAA;
    return 0;
}
