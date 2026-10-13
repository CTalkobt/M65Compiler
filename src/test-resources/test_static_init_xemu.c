/* test_static_init_xemu.c — link-time constant initializers
 *
 * Results at $C0C0 (01 = pass):
 *   [0] static table of label addresses (goto *tbl[i])
 *   [1] local (automatic) table of label addresses
 *   [2] "text" + 1 in a static pointer array; global char * = "abc" + 2
 *   [3] global int *c = &a[3][5]; &s.m; arr + 2; a sub-array a[1]
 *   [4] global const char a[] = "..." is an array (sizeof, contents)
 *   [5] extern declaration followed by an initialized definition
 *   [6] a static label table in an inline function called twice
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct S { int x; char m[4]; };
int a[6][9];
struct S s;
int arr[5];
int *c = &a[3][5];
char *sm = &s.m[1];
int *a2 = arr + 2;
int *row = a[1];
const char *q = "abc" + 2;
const char text[] = "Hi!THE";
extern volatile int ready;
volatile int ready = 1;

static int pick(int i) {
    static const void *tbl[] = { &&zero, &&one, &&two };
    goto *tbl[i];
zero: return 10;
one: return 11;
two: return 12;
}

static int pick2(int i) {
    void *tbl[] = { &&zero, &&one };
    goto *tbl[i];
zero: return 20;
one: return 21;
}

static inline int viaInline(void) {
    static void *t[] = { &&done };
    goto *t[0];
done:
    return 17;
}
static int f1(void) { return viaInline(); }
static int f2(void) { return viaInline(); }

int main(void) {
    static const char *const foo[] = { "foo" + 1 };
    OUT[0] = pick(0) == 10 && pick(1) == 11 && pick(2) == 12;
    OUT[1] = pick2(0) == 20 && pick2(1) == 21;
    OUT[2] = foo[0][0] == 'o' && foo[0][1] == 'o' && foo[0][2] == 0 && q[0] == 'c' && q[1] == 0;
    OUT[3] = c == &a[3][5] && sm == &s.m[1] && a2 == &arr[2] && row == &a[1][0];
    OUT[4] = sizeof(text) == 7 && text[0] == 'H' && text[5] == 'E' && text[6] == 0;
    OUT[5] = ready == 1;
    OUT[6] = f1() == 17 && f2() == 17;
    OUT[7] = 0xAA;
    return 0;
}
