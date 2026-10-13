#pragma cc45 no_zp_save
/* test_struct_copy_xemu.c — struct/union copies of sizes other than 1/2/4
 *
 * Expected at $C0C0: 07 08 09 0A 03 03 07 0A 08 AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
typedef struct { int a; int b; int c; } T3;
typedef struct { char x; char y; char z; } C3;
typedef struct { int k; T3 in; } N;
T3 g1, g2 = { 7, 8, 9 };
C3 c1, c2;
N n1, n2;
T3 *pp;
void main() {
    c2.x = 1; c2.y = 2; c2.z = 3;
    g1 = g2;                                   /* global <- global (memcpy) */
    T3 t = g1;                                  /* local initialised from global */
    t.c = 10;
    g2 = t;                                     /* global <- local */
    c1 = c2;                                    /* 3-byte struct (unrolled) */
    n1.in = g2;                                 /* member that is a struct */
    n2 = n1;
    pp = &g1;
    T3 u = *pp;                                 /* copy through a pointer */
    R(0) = g1.a; R(1) = g1.b; R(2) = g1.c;
    R(3) = g2.c;
    R(4) = c2.z; R(5) = c1.z;
    R(6) = n2.in.a; R(7) = n2.in.c;
    R(8) = u.b;
    R(9) = 0xAA;
}
