/* test_struct_init_xemu.c — constant initializers of aggregate globals and
 * local statics, laid out by the struct layout (not one word per element)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] struct { float; int[3] }        [1] char[N] member from a string
 *   [2] mixed char/int/long members     [3] nested struct + array of structs
 *   [4] brace elision                   [5] .member / [index] designators
 *   [6] union (first / designated)      [7] pointer members (string, &var)
 *   [8] bitfields                       [9] 2-D array
 *  [10] local static struct            [11] (*fp)(...) returning a struct
 *  [12] $AA sentinel
 */
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct { float d; int i[3]; } B;
typedef struct { char c[33], c1; } X;
struct M { char a; int b; long c; };
struct In { int x, y; };
struct Out { char tag; struct In in; struct In arr[2]; };
union U { int i; char c[4]; };
struct Ptr { const char *s; int *p; };
struct Bits { unsigned a : 3, b : 5, c : 8; };

B b1 = {0.5, {1, 2, 3}};
X x1 = {"abcdefghijklmnopqrstuvwxyzABCDEF", 'G'};
struct M m1 = {1, 0x1234, 0x12345678L};
struct Out o1 = {'T', {5, 6}, {{7, 8}, {9, 10}}};
struct Out o2 = {'E', 11, 12, 13, 14, 15, 16};          /* brace elision */
struct Out o3 = {.in = {.y = 21}, .tag = 'D', .arr = {[1] = {23, 24}}};
int arr1[6] = {[4] = 44, [1] = 11, 12};
union U u1 = {0x4142};
union U u2 = {.c = {1, 2, 3, 4}};
int target = 99;
struct Ptr p1 = {"hello", &target};
struct Bits bits = {5, 17, 200};
int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};

X mx(char k) { X r; r.c1 = k; r.c[0] = 'z'; return r; }
X (*fx)(char) = &mx;

int check_static(void) {
    static X xs = {"return val", 'R'};
    static struct In pts[] = {{1, 2}, {3, 4}};
    return strcmp(xs.c, "return val") == 0 && xs.c1 == 'R' && pts[1].x == 3 && pts[1].y == 4;
}

int main(void) {
    OUT[0] = b1.d == 0.5 && b1.i[0] == 1 && b1.i[1] == 2 && b1.i[2] == 3;
    OUT[1] = strcmp(x1.c, "abcdefghijklmnopqrstuvwxyzABCDEF") == 0 && x1.c1 == 'G';
    OUT[2] = m1.a == 1 && m1.b == 0x1234 && m1.c == 0x12345678L;
    OUT[3] = o1.tag == 'T' && o1.in.x == 5 && o1.in.y == 6 && o1.arr[0].y == 8 && o1.arr[1].x == 9;
    OUT[4] = o2.in.x == 11 && o2.in.y == 12 && o2.arr[0].x == 13 && o2.arr[1].y == 16;
    OUT[5] = o3.tag == 'D' && o3.in.x == 0 && o3.in.y == 21 && o3.arr[0].x == 0 && o3.arr[1].y == 24 &&
             arr1[0] == 0 && arr1[1] == 11 && arr1[2] == 12 && arr1[4] == 44;
    OUT[6] = u1.i == 0x4142 && u2.c[3] == 4;
    OUT[7] = strcmp(p1.s, "hello") == 0 && *p1.p == 99;
    OUT[8] = bits.a == 5 && bits.b == 17 && bits.c == 200;
    OUT[9] = grid[0][2] == 3 && grid[1][0] == 4 && grid[1][2] == 6;
    OUT[10] = check_static();
    {
        X r = (*fx)('w');
        OUT[11] = r.c1 == 'w' && r.c[0] == 'z';
    }
    OUT[12] = 0xAA;
    return 0;
}
