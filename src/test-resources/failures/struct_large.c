#pragma cc45 no_zp_save
/* Structs larger than 4 bytes: assignment and return. Expected at $C0C0: 44 11 88 55 07 08 09 07 08 09 AA */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
typedef struct { signed long a; signed long b; } P;
P mk(signed long x, signed long y) { P p; p.a = x; p.b = y; return p; }
typedef struct { int a; int b; int c; } T3;
T3 mk3(int x) { T3 t; t.a = x; t.b = x + 1; t.c = x + 2; return t; }
T3 g1, g2;
void main() {
    P p = mk(0x11223344L, 0x55667788L);
    R(0) = (unsigned char)p.a; R(1) = (unsigned char)(p.a >> 24); R(2) = (unsigned char)p.b; R(3) = (unsigned char)(p.b >> 24);
    T3 t = mk3(7); R(4) = t.a; R(5) = t.b; R(6) = t.c;
    g1 = t; g2 = g1; R(7) = g2.a; R(8) = g2.b; R(9) = g2.c;
    R(10) = 0xAA;
}
