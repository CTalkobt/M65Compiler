/* test_local_init_xemu.c — local aggregate initializers, preprocessor fixes
 *
 * Results at $C0C0 (01 = pass):
 *   [0] constant local struct with nested array, local union with array
 *   [1] run-time values: nested structs, struct-valued member, string into
 *       char array, float and pointer members
 *   [2] designators: nested (.b.y, .a.x), array index, mixed with elision
 *   [3] 2-D local array with brace elision, array of pointers to locals
 *   [4] 2-byte struct compound literals as members
 *   [5] #define F / #define L don't touch 1.0F, 10L, 'F'; hex in #if
 *   [6] __FLT_MAX__ / __FLT_MIN__ ordering; (double)1 is a float constant
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define F 140
#define L 7

struct P { signed int x, y; };
struct R { struct P a, b; char name[6]; float f; char *s; };
struct T { unsigned short rep[4]; char c; };
union U { unsigned short w[2]; long l; };
typedef struct { short v; } J;
struct N { J m; J t; int len; };

#if 0x7fffffff > 32767 && 10UL == 10
#define HEX_IF_OK 1
#else
#define HEX_IF_OK 0
#endif

int main(void) {
    volatile signed int v = 7;
    int k = 3;
    struct T t = {{ 1, 2, 3, 4 }, 5};
    union U u = {{ 0x1122, 0x3344 }};
    struct P p0 = { 1, 2 };
    struct R r = { { v, -v }, p0, "ab", 2.5, "xyz" };
    struct R r2 = { .b = { .y = 9 }, .name = "hi", .a.x = 4 };
    struct P arr[3] = { { 1, 2 }, [2].y = 5, [2].x = v };
    int m[2][3] = { 1, 2, 3, 4, 5, v };
    int *ptrs[2] = { &k, 0 };
    struct N n = { .m = (J){ 0x1985 }, .t = (J){ 0x2003 }, .len = k };
    float vals[3] = { __FLT_MIN__, 1.0F, __FLT_MAX__ };

    OUT[0] = t.rep[0] == 1 && t.rep[3] == 4 && t.c == 5 && u.w[0] == 0x1122 && u.w[1] == 0x3344;
    OUT[1] = r.a.x == 7 && r.a.y == -7 && r.b.x == 1 && r.b.y == 2 && r.name[1] == 'b' && r.name[2] == 0 &&
             r.f == 2.5 && r.s[2] == 'z';
    OUT[2] = r2.b.y == 9 && r2.b.x == 0 && r2.name[0] == 'h' && r2.a.x == 4 && r2.a.y == 0 && r2.s == 0 &&
             arr[0].y == 2 && arr[1].x == 0 && arr[2].x == 7 && arr[2].y == 5;
    OUT[3] = m[0][2] == 3 && m[1][0] == 4 && m[1][2] == 7 && *ptrs[0] == 3 && ptrs[1] == 0;
    OUT[4] = n.m.v == 0x1985 && n.t.v == 0x2003 && n.len == 3;
    OUT[5] = 1.0F == 1.0 && 10L == 10 && 'F' != 140 && F == 140 && L == 7 && HEX_IF_OK;
    OUT[6] = vals[0] > 0.0F && vals[0] < vals[1] && vals[1] < vals[2] && (double)1 == 1.0 && (float)-2 == -2.0;
    OUT[7] = 0xAA;
    return 0;
}
