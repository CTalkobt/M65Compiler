/* test_struct_layout_xemu.c — sizeof and member layout agree everywhere
 *
 * sizeof(type) (constant folding), sizeof in an array dimension (parser), a
 * variable's storage and member offsets (code generation) must give a struct
 * the same layout: __unpacked structs (natural alignment, tail padding),
 * bitfields, nested structs, flexible array members, typedefs.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __unpacked sizes include the tail padding (4, 4, 12)
 *   [1] __unpacked member offsets (offsetof and addresses)
 *   [2] arrays of an __unpacked struct: stride and sizeof
 *   [3] nested: an __unpacked struct in an __unpacked / packed struct
 *   [4] bitfield struct: sizeof(type) == sizeof variable
 *   [5] sizeof(struct) in array dimensions (global and local)
 *   [6] struct __unpacked Name in declarations (global, local, member, parameter)
 *   [7] flexible array member: not counted in sizeof
 *   [8] typedefs of an __unpacked struct and of an array of it
 *   [9] __unpacked bitfields: the unit is aligned
 *   [10] values stored in an array of an __unpacked struct read back
 *   [11] $AA sentinel
 */
#include <stddef.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define DIFF(a, b) ((unsigned)((char *)(a) - (char *)(b)))

struct __unpacked U1 { char a; int b; };
struct __unpacked U2 { int b; char a; };
struct __unpacked U3 { char a; long l; char c; };
struct __unpacked U4 { char a; struct U3 s; char z; };
struct W { char x; struct __unpacked U3 s; };
struct B { unsigned int a : 3; unsigned int b : 5; unsigned char c; };
struct F { int n; char d[]; };
typedef struct __unpacked { char a; int b; } T;
typedef T TA[3];
struct __unpacked UB { char a; unsigned int b : 3; unsigned int c : 4; };

struct U1 g1; struct U2 g2; struct U3 g3;
struct U3 a3[2];
struct U4 g4;
struct W gw;
struct B gb;
struct __unpacked U3 gdecl;
struct F gf;
T gt; TA gta;
struct UB gub;

char buf_b[sizeof(struct B)];
char buf_u1[sizeof(struct U1)];
char buf_u3[sizeof(struct U3)];
char buf_u4[sizeof(struct U4)];
char buf_ta[sizeof(TA)];

static unsigned char sum3(struct __unpacked U3 *p) { return p->a + p->c + (unsigned char)p->l; }

int main(void) {
    unsigned char ok;
    struct __unpacked U3 local;

    ok = sizeof(struct U1) == 4 && sizeof(struct U2) == 4 && sizeof(struct U3) == 12 &&
         sizeof g1 == 4 && sizeof g2 == 4 && sizeof g3 == 12;
    OUT[0] = ok;

    ok = offsetof(struct U1, b) == 2 && offsetof(struct U3, l) == 4 && offsetof(struct U3, c) == 8 &&
         DIFF(&g1.b, &g1) == 2 && DIFF(&g3.l, &g3) == 4 && DIFF(&g3.c, &g3) == 8;
    OUT[1] = ok;

    ok = DIFF(&a3[1], &a3[0]) == 12 && sizeof a3 == 24 && sizeof a3 / sizeof a3[0] == 2;
    OUT[2] = ok;

    ok = sizeof(struct U4) == 20 && sizeof g4 == 20 && offsetof(struct U4, s) == 4 &&
         offsetof(struct U4, z) == 16 && DIFF(&g4.s.l, &g4) == 8 && DIFF(&g4.z, &g4) == 16 &&
         sizeof(struct W) == 13 && sizeof gw == 13 && DIFF(&gw.s.c, &gw) == 9;
    OUT[3] = ok;

    ok = sizeof(struct B) == 3 && sizeof gb == 3 && offsetof(struct B, c) == 2;
    OUT[4] = ok;

    {
        char lb[sizeof(struct U3)];
        char lb2[sizeof(struct B) * 2];
        ok = sizeof buf_b == 3 && sizeof buf_u1 == 4 && sizeof buf_u3 == 12 && sizeof buf_u4 == 20 &&
             sizeof lb == 12 && sizeof lb2 == 6;
    }
    OUT[5] = ok;

    local.a = 1; local.l = 2; local.c = 3;
    gdecl = local;
    ok = sizeof local == 12 && sizeof gdecl == 12 && sizeof(struct __unpacked U3) == 12 &&
         sum3(&gdecl) == 6 && sum3(&local) == 6;
    OUT[6] = ok;

    ok = sizeof(struct F) == 2 && sizeof gf == 2 && offsetof(struct F, d) == 2;
    OUT[7] = ok;

    ok = sizeof(T) == 4 && sizeof gt == 4 && sizeof(TA) == 12 && sizeof gta == 12 &&
         sizeof buf_ta == 12 && DIFF(&gta[2], &gta[0]) == 8 && DIFF(&gt.b, &gt) == 2;
    OUT[8] = ok;

    gub.a = 0x5A; gub.b = 5; gub.c = 9;
    ok = sizeof(struct UB) == 4 && sizeof gub == 4 && gub.a == 0x5A && gub.b == 5 && gub.c == 9 &&
         ((unsigned char *)&gub)[2] == (5 | (9 << 3));
    OUT[9] = ok;

    {
        unsigned char i;
        for (i = 0; i < 2; i++) { a3[i].a = i + 1; a3[i].l = 0x10000L * (i + 1) + 7; a3[i].c = 0x40 + i; }
        ok = a3[0].a == 1 && a3[1].a == 2 && a3[0].l == 0x10007L && a3[1].l == 0x20007L &&
             a3[0].c == 0x40 && a3[1].c == 0x41 && ((unsigned char *)a3)[12] == 2 &&
             ((unsigned char *)a3)[20] == 0x41;
    }
    OUT[10] = ok;

    OUT[11] = 0xAA;
    return 0;
}
