/* test_struct_asm_xemu.c — nested struct definitions, zero-length arrays,
 * extended asm operands, constant SAC parameters that are written
 *
 * Results at $C0C0 (01 = pass):
 *   [0] struct S { struct T {...} t; } u; then struct T used on its own
 *   [1] union with an anonymous struct member; struct with only a [0] array
 *   [2] asm with :: and inputs that have side effects
 *   [3] asm outputs tied to inputs ("=r" : "0"), "+r" lvalue evaluated once
 *   [4] a parameter passed the same constant at every call, written through
 *       its address in the callee
 *   [5..6] 01
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct S { struct T { char a; int b; } t; char c; } u;
static int getb(struct T *p) { return p->b; }

union U { struct { int i1, i2; } t; long l; } x[2];
typedef struct { char d[0]; } Z;

static int calls;
static int *cell(void) { static int c = 5; calls++; return &c; }

static int set_via_addr(int a, int b) { int *p = &a; *p = a + b; return a; }

int main(void) {
    int r, q;

    u.t.b = 1234;
    OUT[0] = getb(&u.t) == 1234 && sizeof(struct T) == 3;

    x[1].t.i1 = 7; x[1].t.i2 = 9;
    OUT[1] = x[1].t.i2 == 9 && sizeof(union U) == 4 && sizeof(Z) == 0;

    calls = 0;
    asm volatile ("" :: "r" (*cell()), "m" (r) : "memory");
    OUT[2] = calls == 1;

    calls = 0;
    asm ("" : "=r" (r) : "0" (42));
    asm ("" : "+r" (*cell()));
    q = 3;
    asm volatile ("" : "=r" (q) : "0" (q + 1));
    OUT[3] = r == 42 && calls == 1 && *cell() == 5 && q == 4;

    OUT[4] = set_via_addr(16, 1) == 17 && set_via_addr(16, 2) == 18;

    OUT[5] = 1;
    OUT[6] = 1;
    OUT[7] = 0xAA;
    return 0;
}
