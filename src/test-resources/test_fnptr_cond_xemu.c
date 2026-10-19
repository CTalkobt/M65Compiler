/* test_fnptr_cond_xemu.c — calls in a ?: (Issue #307; found by far_malloc's
 * attic hooks, #306):
 *   - a call through a function pointer has its pointer's result type: in a
 *     ?: (or any expression typed before it is built) a long result kept
 *     only its low 16 bits
 *   - an argument of a call in an arm of a struct-valued ?: is a value: a
 *     parameter was passed as its address
 *
 * Results at $C0C0 (01 = pass):
 *   [0] p ? hook(a) : 0, hook a global unsigned long (*)(unsigned long)
 *   [1] the same through an array element, a struct member, a parameter
 *   [2] (*hook)(a) in a ?:, and (?:) + 1
 *   [3] a char * result through a pointer in a ?: (pointer arithmetic)
 *   [4] k ? fp(size) : z, fp returning a struct, size a 16-bit parameter
 *   [5] the same with a direct call, in either arm
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

typedef struct { unsigned char b[3]; } fp_t;

static unsigned long big(unsigned long v) { return v << 4; }
static char text[] = "abcdef";
static char *at(unsigned int i) { return text + i; }
static fp_t idf(unsigned long v) { fp_t r; r.b[0] = v; r.b[1] = v >> 8; r.b[2] = v >> 16; return r; }
static fp_t idi(unsigned int v, unsigned char c) { fp_t r; r.b[0] = v; r.b[1] = v >> 8; r.b[2] = c; return r; }

unsigned long (*hook)(unsigned long);
unsigned long (*hooks[2])(unsigned long);
struct ops { unsigned long (*f)(unsigned long); } ops;
char *(*ptrf)(unsigned int);
fp_t (*hookf)(unsigned long);

NOINLINE static unsigned long g1(unsigned long a) { return hook ? hook(a) : 0; }
NOINLINE static unsigned long g2(unsigned long a) { return hooks[1] ? hooks[1](a) : 0; }
NOINLINE static unsigned long g3(unsigned long a) { return ops.f ? ops.f(a) : 0; }
NOINLINE static unsigned long g4(unsigned long a, unsigned long (*p)(unsigned long)) { return p ? p(a) : 0; }
NOINLINE static unsigned long g5(unsigned long a) { return hook ? (*hook)(a) : 0; }
NOINLINE static unsigned long g6(unsigned long a) { return (hook ? hook(a) : 0) + 1; }
NOINLINE static char g7(unsigned int i) { return *((ptrf ? ptrf(i) : text) + 1); }
NOINLINE static fp_t s1(unsigned int size, unsigned char k) {
    fp_t z = {{0, 0, 0}};
    return k && hookf ? hookf(size) : z;
}
NOINLINE static fp_t s2(unsigned int size, unsigned char k) {
    fp_t z = {{0, 0, 0}};
    return k ? idf(size) : z;
}
NOINLINE static fp_t s3(unsigned int size, unsigned char c, unsigned char k) {
    fp_t z = {{0, 0, 0}};
    return k ? z : idi(size, c);
}

int main(void) {
    fp_t r;
    hook = big; hooks[1] = big; ops.f = big; ptrf = at; hookf = idf;
    OUT[0] = g1(0x8000UL) == 0x80000UL;
    OUT[1] = g2(0x8000UL) == 0x80000UL && g3(0x8000UL) == 0x80000UL && g4(0x8000UL, big) == 0x80000UL;
    OUT[2] = g5(0x8000UL) == 0x80000UL && g6(0x8000UL) == 0x80001UL;
    OUT[3] = g7(2) == 'd';
    r = s1(10000, 1);
    OUT[4] = r.b[0] == 0x10 && r.b[1] == 0x27 && r.b[2] == 0;
    r = s2(10000, 1);
    OUT[5] = r.b[0] == 0x10 && r.b[1] == 0x27 && r.b[2] == 0;
    r = s3(0x1234, 9, 0);
    OUT[5] = OUT[5] && r.b[0] == 0x34 && r.b[1] == 0x12 && r.b[2] == 9;
    OUT[6] = 0xAA;
    return 0;
}
