/* test_shadow_slot_xemu.c -- a variable reached through a pointer keeps its
 * frame slot to the end of the function (#664)
 *
 * The allocator keeps the variables it knows by name (localNames) for the
 * whole function. A name declared again -- in an inner scope, or by an inlined
 * copy of the function's own body -- leaves the outer one out of that map,
 * and its slot was freed after its last mention: the next variable of the same
 * size took it while a pointer to the first was still in use.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] char b[4] declared three times in nested scopes, the outer two read
 *       through pointers at the end
 *   [1] the same with char b[2] (an int's size)
 *   [2] an int whose address is taken, declared three times
 *   [3] [0] in a function with a stack frame (#pragma cc45 recurse)
 *   [4] a recursive function inlined into itself: its array read through a
 *       pointer after the call
 *   [5] $AA sentinel
 */
volatile unsigned char *const R = (volatile unsigned char *)0xC0C0;

unsigned char shadow4(unsigned char n) {
    unsigned char s;
    char b[4];
    char *p = b;
    p[0] = n; p[1] = n + 1; p[2] = n + 2; p[3] = n + 3;
    {
        char b[4];
        char *q = b;
        q[0] = 10; q[1] = 20; q[2] = 30; q[3] = 40;
        {
            char b[4];
            b[0] = q[0] + p[0];
            b[3] = q[3];
            s = b[0] + b[3];
        }
        s += q[1];
    }
    return s + p[0] + p[1] + p[2] + p[3];
}

unsigned char shadow2(unsigned char n) {
    unsigned char s;
    char b[2];
    char *p = b;
    p[0] = n; p[1] = n + 1;
    {
        char b[2];
        char *q = b;
        q[0] = 50; q[1] = 60;
        {
            char b[2];
            b[0] = q[0] + p[0];
            b[1] = q[1];
            s = b[0] + b[1];
        }
        s += q[1];
    }
    return s + p[0] + p[1];
}

unsigned int shadow_int(unsigned int n) {
    unsigned int s;
    unsigned int x;
    unsigned int *px = &x;
    *px = n;
    {
        unsigned int x;
        unsigned int *qx = &x;
        *qx = 1000;
        {
            unsigned int x;
            unsigned int *rx = &x;
            *rx = *qx + *px;
            s = *rx;
        }
        s += *qx;
    }
    return s + *px;
}

#pragma cc45 recurse
unsigned char shadow4_frame(unsigned char n) {
    unsigned char s;
    char b[4];
    char *p = b;
    p[0] = n; p[1] = n + 1; p[2] = n + 2; p[3] = n + 3;
    {
        char b[4];
        char *q = b;
        q[0] = 10; q[1] = 20; q[2] = 30; q[3] = 40;
        {
            char b[4];
            b[0] = q[0] + p[0];
            b[3] = q[3];
            s = b[0] + b[3];
        }
        s += q[1];
    }
    return s + p[0] + p[1] + p[2] + p[3];
}

#pragma cc45 recurse
unsigned char rec(unsigned char n) {
    char b[8];
    char *p = b;
    unsigned char i, s = 0;
    for (i = 0; i < 8; i++) p[i] = n + i;
    if (n) s = rec(n - 1);
    for (i = 0; i < 8; i++) s += p[i];
    return s;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 6; i++) R[i] = 0;
    R[0] = shadow4(1) == 81;
    R[1] = shadow2(1) == 174;
    R[2] = shadow_int(7) == 2014;
    R[3] = shadow4_frame(1) == 81;
    R[4] = rec(5) == 32;
    R[5] = 0xAA;
    return 0;
}
