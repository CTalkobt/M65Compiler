/* test_struct_dims_xemu.c — array sizes of the struct members after the
 * first in a declarator list are constant expressions (#563: they had to be
 * integer literals)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the structs' sizes (a sum, parentheses, an enum constant, sizeof of
 *       a type and of a variable, two dimensions, a pointer array)
 *   [1] the members' offsets and their elements written and read
 *   [2] $AA sentinel
 */
#include <stddef.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define RLEN (sizeof g + 1)

enum { N = 5 };
char g[9];

struct b { char p, a[4 + 1]; };
struct c { char p, a[(4)]; };
struct d { char p, a[N]; };
struct e { char p, a[sizeof(int)]; };
struct f { char p, a[sizeof g], z[RLEN]; };
struct h { char a[2], b[4 + 1], c[N][2 * N], *q[N - 1]; int w, v[N + N]; };
union u { char p, a[N * 2]; long l, m[sizeof(int)]; };

struct h hv;

int main(void) {
    unsigned char i, ok;

    OUT[0] = sizeof(struct b) == 6 && sizeof(struct c) == 5 && sizeof(struct d) == 6 &&
             sizeof(struct e) == 3 && sizeof(struct f) == 20 &&
             sizeof(struct h) == 2 + 5 + 50 + 8 + 2 + 20 && sizeof(union u) == 10;

    ok = offsetof(struct h, b) == 2 && offsetof(struct h, c) == 7 && offsetof(struct h, q) == 57 &&
         offsetof(struct h, w) == 65 && offsetof(struct h, v) == 67 && offsetof(struct f, z) == 10;
    for (i = 0; i < 5; i++) hv.b[i] = i + 1;
    hv.c[4][9] = 0x5A;
    hv.q[3] = &hv.b[2];
    hv.v[9] = 0x1234;
    hv.w = 0x4321;
    ok = ok && hv.a[1] == 0 && hv.b[4] == 5 && *hv.q[3] == 3 && hv.c[4][9] == 0x5A &&
         ((char *)&hv)[56] == 0x5A && hv.v[9] == 0x1234 && hv.w == 0x4321;
    OUT[1] = ok;
    OUT[2] = 0xAA;
    return 0;
}
