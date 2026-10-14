/* test_sizeof_xemu.c — sizeof of arrays of pointers and of array type names
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sizeof an array of pointers (global, static, local): elements are
 *       pointers (2 bytes), not the pointee; sizeof a / sizeof a[0]
 *   [1] struct members: arrays (of pointers too), a struct, a char, a
 *       pointer, through a pointer (p->b); the struct's size
 *   [2] array type names: sizeof(int[4]), (char *[4]), (cp[3]),
 *       (struct S[2]), (long[2][3])
 *   [3] a pointer to an array: sizeof(int (*)[4]); typedef'd arrays:
 *       sizeof(arr4), (arr4 *), (arr4[2])
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
typedef char *cp;
typedef int arr4[4];
struct S { int a; };
struct In { int x; long y; };
struct M { char *a[3]; int b[4]; long *c[2]; struct In in; char ch; int *p; };
static const char *f3[] = {"a", "b", "c"};
static char *g4[4];
static struct M m;
static struct M *pm = &m;

int main(void) {
    char *loc[5];
    int *li[3];
    (void)loc; (void)li;
    OUT[0] = sizeof f3 == 6 && sizeof f3 / sizeof f3[0] == 3 && sizeof g4 == 8 && sizeof loc == 10 &&
             sizeof li == 6 && sizeof loc / sizeof loc[0] == 5;
    OUT[1] = sizeof m.a == 6 && sizeof m.b == 8 && sizeof m.c == 4 && sizeof m.in == 6 && sizeof m.ch == 1 &&
             sizeof m.p == 2 && sizeof pm->b == 8 && sizeof(struct M) == 27;
    OUT[2] = sizeof(int[4]) == 8 && sizeof(char *[4]) == 8 && sizeof(cp[3]) == 6 &&
             sizeof(struct S[2]) == 4 && sizeof(long[2][3]) == 24;
    OUT[3] = sizeof(int (*)[4]) == 2 && sizeof(arr4) == 8 && sizeof(arr4 *) == 2 && sizeof(arr4[2]) == 16;
    OUT[4] = 0xAA;
    return 0;
}
