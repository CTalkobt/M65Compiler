/* test_typedef_multi_xemu.c — a typedef with several declarators
 * (typedef struct {...} R, *RP; dhrystone's Rec_Type, *Rec_Pointer)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] struct, pointer and array names of one typedef: their sizes
 *   [1] the pointer name used: p->n->v.x
 *   [2] typedef int I, *IP, (*F)(int), IA[3]: a function pointer among them
 *   [3] the array names: elements written and read
 *   [4] a typedef in a function, two names
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct r { struct r *n; union { int x; char c[3]; } v; } R, *RP, RA[2];
typedef int I, *IP, (*F)(int), IA[3];

static int twice(int a) { return a * 2; }

R g;
RA arr;

int main(void) {
    RP p = &g;
    I i = 5;
    IP ip = &i;
    F f = twice;
    IA a;
    typedef char C, *CP;
    C c = 'x';
    CP cp = &c;

    OUT[0] = (sizeof(RA) == 2 * sizeof(R) && sizeof(RP) == 2 && sizeof(R) == 5 && sizeof(IA) == 6) ? 1 : 0;
    p->v.x = 7;
    p->n = p;
    OUT[1] = p->n->v.x == 7 ? 1 : 0;
    OUT[2] = (f(*ip) == 10 && sizeof(IP) == 2) ? 1 : 0;
    a[2] = 3;
    arr[1].v.x = a[2];
    OUT[3] = arr[1].v.x == 3 ? 1 : 0;
    OUT[4] = (*cp == 'x' && sizeof(C) == 1 && sizeof(CP) == 2) ? 1 : 0;
    OUT[5] = 0xAA;
    return 0;
}
