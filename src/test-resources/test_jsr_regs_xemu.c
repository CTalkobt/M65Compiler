/* test_jsr_regs_xemu.c — __jsr with several register values that are not
 * constants: each register loaded from where its value lives (a variable in
 * the base page or static storage, a constant after inlining), or through
 * its __jsr byte when the value has no address or may change before the call
 *
 * Results at $C0C0 (01 = pass):
 *   [0] locals to .A .X .Y .Z, alone and mixed with constants
 *   [1] expressions and globals as values
 *   [2] .XY, .AX, .AY, .AZ and .Q from variables, with 8-bit registers beside
 *   [3] a function with a stack frame (#pragma cc45 recurse): frame locals
 *   [4] a later value changes a variable an earlier value read (an
 *       assignment, a function call): the earlier register keeps what it read
 *   [5] a __jsr in a value of another __jsr
 *   [6] always-inlined and called wrappers, constant and variable arguments
 *   [7] a volatile local, a char widened to .XY, the same variable twice
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

extern void cap(void);        /* A X Y Z stored in ca cx cy cz; Z = 0 again */
extern void twice(void);      /* A = A * 2 */
extern volatile unsigned char ca, cx, cy, cz;

asm("_cap:\n sta _ca\n stx _cx\n sty _cy\n stz _cz\n ldz #0\n rts\n"
    "_twice:\n asl\n rts\n"
    "_ca: .byte 0\n_cx: .byte 0\n_cy: .byte 0\n_cz: .byte 0\n");

unsigned char ga = 0x21, gb = 0x32;
unsigned int gw = 0x4354;
unsigned char bumped = 7;

static unsigned char is(unsigned char a, unsigned char x, unsigned char y, unsigned char z) {
    return ca == a && cx == x && cy == y && cz == z;
}

#pragma cc45 optimize(off)
static unsigned char bump(void) { bumped += 3; return bumped; }
#pragma cc45 optimize(on)

static __forceinline void put3(unsigned char a, unsigned char x, unsigned char y) {
    __jsr_raw(cap, .A = a, .X = x, .Y = y, .Z = 0);
}
static __attribute__((noinline)) void call3(unsigned char a, unsigned int xy) {
    __jsr_raw(cap, .XY = xy, .A = a, .Z = 0);
}

static unsigned char t0(void) {
    unsigned char a = 1, x = 2, y = 3, z = 4, ok;
    __jsr_raw(cap, .A = a, .X = x, .Y = y, .Z = z);
    ok = is(1, 2, 3, 4);
    __jsr_raw(cap, .Y = a, .Z = x, .A = y, .X = z);
    ok = ok && is(3, 4, 1, 2);
    __jsr_raw(cap, .A = 9, .X = x, .Y = 8, .Z = z);
    ok = ok && is(9, 2, 8, 4);
    __jsr_raw(cap, .X = y, .Y = a, .A = 0, .Z = 0);
    return ok && is(0, 3, 1, 0);
}

static unsigned char t1(void) {
    unsigned char a = 5, b = 6, ok;
    __jsr_raw(cap, .A = a + b, .X = (unsigned char)(b << 2), .Y = ga, .Z = gb);
    ok = is(11, 24, 0x21, 0x32);
    __jsr_raw(cap, .A = ga + 1, .X = gb, .Y = a ^ b, .Z = 0);
    return ok && is(0x22, 0x32, 3, 0);
}

static unsigned char t2(void) {
    unsigned int w = 0x1234;
    unsigned long q = 0x0A0B0C0DUL;
    unsigned char c = 0x56, ok;
    __jsr_raw(cap, .XY = w, .A = c, .Z = 0);
    ok = is(0x56, 0x34, 0x12, 0);
    __jsr_raw(cap, .AX = w, .Y = c, .Z = c);
    ok = ok && is(0x34, 0x12, 0x56, 0x56);
    __jsr_raw(cap, .AY = gw, .X = c, .Z = 0);
    ok = ok && is(0x54, 0x56, 0x43, 0);
    __jsr_raw(cap, .AZ = w, .X = c, .Y = 1);
    ok = ok && is(0x34, 0x56, 1, 0x12);
    __jsr_raw(cap, .Q = q);
    ok = ok && is(0x0D, 0x0C, 0x0B, 0x0A);
    __jsr_raw(cap, .XY = w + 1, .A = c, .Z = 0);
    return ok && is(0x56, 0x35, 0x12, 0);
}

#pragma cc45 recurse
static unsigned char t3(unsigned char depth, unsigned int w) {
    unsigned char a = depth + 10, x = depth + 20, y = depth + 30, ok;
    if (depth) return t3(depth - 1, w + 1);
    __jsr_raw(cap, .A = a, .X = x, .Y = y, .Z = x);
    ok = is(10, 20, 30, 20);
    __jsr_raw(cap, .XY = w, .A = y, .Z = 0);
    return ok && is(30, 0x03, 0x70, 0);
}

static unsigned char t4(void) {
    unsigned char i = 4, ok;
    __jsr_raw(cap, .X = i, .Y = (i = i + 5), .A = i, .Z = 0);
    ok = is(9, 4, 9, 0);
    __jsr_raw(cap, .X = bumped, .Y = bump(), .A = bumped, .Z = 0);
    return ok && is(10, 7, 10, 0);
}

static unsigned char t5(void) {
    unsigned char v = 0x11, n = 0x20;
    __jsr_raw(cap, .X = v, .A = __jsr_raw(twice, .A = n).A, .Y = n, .Z = 0);
    return is(0x40, 0x11, 0x20, 0);
}

static unsigned char t6(void) {
    unsigned char a = 0x61, b = 0x62, ok;
    put3(1, 8, 2);
    ok = is(1, 8, 2, 0);
    put3(a, b, a);
    ok = ok && is(0x61, 0x62, 0x61, 0);
    put3(b, 3, ga);
    ok = ok && is(0x62, 3, 0x21, 0);
    call3(a, 0x9876);
    ok = ok && is(0x61, 0x76, 0x98, 0);
    call3(7, gw);
    return ok && is(7, 0x54, 0x43, 0);
}

static unsigned char t7(void) {
    volatile unsigned char v = 0x71;
    unsigned char c = 0x82, ok;
    __jsr_raw(cap, .A = v, .X = c, .Y = v, .Z = 0);
    ok = is(0x71, 0x82, 0x71, 0);
    __jsr_raw(cap, .XY = c, .A = c, .Z = c);
    return ok && is(0x82, 0x82, 0, 0x82);
}

int main(void) {
    OUT[0] = t0();
    OUT[1] = t1();
    OUT[2] = t2();
    OUT[3] = t3(2, 0x7001);
    OUT[4] = t4();
    OUT[5] = t5();
    OUT[6] = t6();
    OUT[7] = t7();
    OUT[8] = 0xAA;
    return 0;
}
