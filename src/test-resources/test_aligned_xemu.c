/* test_aligned_xemu.c — __attribute__((aligned(N))) and _Alignas
 *
 * Results at $C0C0 (01 = pass):
 *   [0] BSS global aligned(8)            [1] initialized global aligned(16)
 *   [2] _Alignas(16) BSS array           [3] member aligned(8) inside an outer struct
 *   [4] aligned(4) struct: sizeof 4      [5] its array elements stay aligned
 *   [6] _Alignof(aligned struct) == 4    [7] local (static storage) aligned(16)
 *   [8] local static aligned(8)
 *   [9] _Alignas anywhere among the declaration specifiers: before the
 *       storage class, after the type, after const, in a struct member and
 *       a local static
 *   [10] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define AL(p, n) ((((unsigned)(p)) & ((n) - 1)) == 0)

char pad1;
int bx __attribute__((aligned(8)));
char pad2 = 1;
int dx __attribute__((aligned (16))) = 5;
char pad3;
_Alignas(16) char ab[3];

struct s1 { int __attribute__((aligned (8))) a; };
struct { char c; struct s1 m; } v;

struct __attribute__((aligned(4))) S4 { char c; };
struct S4 arr4[3];

char pad4;
_Alignas(32) static unsigned char o1[3];
char pad5;
static unsigned char _Alignas(32) o2[3];
char pad6 = 1;
const _Alignas(16) int o3 = 7;
struct o4s { char c; int _Alignas(8) x; } o4;

int main(void) {
    char padl = 1;
    char loc[5] __attribute__((aligned(16)));
    static char st[2] __attribute__((aligned(8)));
    loc[0] = padl;
    OUT[0] = AL(&bx, 8);
    OUT[1] = AL(&dx, 16) && dx == 5;
    OUT[2] = AL(ab, 16);
    OUT[3] = AL(&v.m, 8);
    OUT[4] = sizeof(struct S4) == 4;
    OUT[5] = AL(&arr4[0], 4) && AL(&arr4[1], 4) && AL(&arr4[2], 4);
    OUT[6] = _Alignof(struct S4) == 4;
    OUT[7] = AL(loc, 16) && loc[0] == 1;
    OUT[8] = AL(st, 8);
    {
        _Alignas(16) static unsigned char o5[2];
        OUT[9] = AL(o1, 32) && AL(o2, 32) && AL(&o3, 16) && o3 == 7 && AL(&o4.x, 8) && AL(o5, 16);
    }
    OUT[10] = 0xAA;
    return pad1 + pad2 + pad3 + pad4 + pad5 + pad6 + o1[0] + o2[0];
}
