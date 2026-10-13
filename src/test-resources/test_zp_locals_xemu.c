/* test_zp_locals_xemu.c — pointer variables of functions that make calls in
 * base-page blocks of their own ($80-$E9, placed by ln45)
 *
 * Linked with test_zp_locals_b.c. A static-allocation function that calls
 * others keeps its pointer variables (and, in the ZP convention, its pointer
 * parameters) in a base-page block that no function active at the same time
 * uses: the pointers survive the calls.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a pointer walked across calls in a loop
 *   [1] a chain: each function keeps pointers across a call to the next,
 *       which has pointers of its own
 *   [2] calls into the other module (its functions have blocks too)
 *   [3] a function with a block called through a function pointer from a
 *       function with a block
 *   [4] a recursive function (a stack frame, no block) calling one with a block
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned char *b_find(unsigned char *p, unsigned char c);
void b_upper(unsigned char *d, const unsigned char *s);

NOINLINE static unsigned char twice(unsigned char x) { return (unsigned char)(x * 2); }

NOINLINE static unsigned walk(const unsigned char *p, unsigned char n) {
    unsigned s = 0;
    while (n--) s += twice(*p++);
    return s;
}

NOINLINE static unsigned level3(const unsigned char *p) {
    const unsigned char *q = p + 1;
    return walk(p, 2) + *q;
}

NOINLINE static unsigned level2(const unsigned char *p) {
    const unsigned char *a = p, *b = p + 2;
    unsigned r = level3(b);
    return r + *a + *b;                 /* a and b after the call */
}

NOINLINE static unsigned level1(const unsigned char *p) {
    const unsigned char *x = p + 1;
    unsigned r = level2(x);
    return r + *x + *p;
}

NOINLINE static unsigned char first_of(const unsigned char *p) { unsigned char *q = (unsigned char *)p; return twice(*q); }

static unsigned char (*fp)(const unsigned char *) = first_of;

NOINLINE static unsigned via_ptr(const unsigned char *p, unsigned char n) {
    unsigned s = 0;
    while (n--) s += fp(p++);
    return s;
}

NOINLINE static unsigned rec(const unsigned char *p, unsigned char n) {
    if (!n) return 0;
    return walk(p, 1) + rec(p + 1, n - 1);
}

static unsigned char data[6] = {1, 2, 3, 4, 5, 0};
static unsigned char text[] = {10, 20, 99, 30, 0};
static unsigned char out[8];

int main(void) {
    unsigned char *f;

    OUT[0] = walk(data, 5) == 30;
    /* level1(data): x = data+1 (2); level2: a = 2, b = data+3 (4);
       level3(data+3): walk(4,5) = 18, q = 5 -> 23; +a +b = 29; +x +data[0] = 32 */
    OUT[1] = level1(data) == 32;
    f = b_find(text, 99);
    b_upper(out, text);
    OUT[2] = f == text + 2 && out[0] == 11 && out[2] == 100 && out[3] == 31 && out[4] == 0;
    OUT[3] = via_ptr(data, 3) == 12;
    OUT[4] = rec(data, 3) == 12;
    OUT[5] = 0xAA;
    return 0;
}
