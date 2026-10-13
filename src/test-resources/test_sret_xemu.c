/* test_sret_xemu.c — struct/union results through the caller's hidden result
 * pointer, and struct arguments passed as a copy, on every call path.
 *
 * Expected at $C0C0: 44 11 88 55 07 08 09 07 08 09 0A 14 0B 0D 0F 02 06 05 07 06 07 AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
typedef struct { signed long a; signed long b; } P;
P mk(signed long x, signed long y) { P p; p.a = x; p.b = y; return p; }
typedef struct { int a; int b; int c; } T3;
T3 mk3(int x) { T3 t; t.a = x; t.b = x + 1; t.c = x + 2; return t; }
T3 g1, g2;
struct Point { char x; char y; };
struct Point gp(char px, char py) { struct Point p; p.x = px; p.y = py; return p; }
struct V { int x, y, z;
    struct V operator+(struct V o) { struct V r; r.x = this->x + o.x; r.y = this->y + o.y; r.z = this->z + o.z; return r; }
    struct V twice() { struct V r; r.x = this->x * 2; r.y = this->y * 2; r.z = this->z * 2; return r; }
};
struct V mkv(int a) { struct V r; r.x = a; r.y = a + 1; r.z = a + 2; return r; }
int sumv(struct V o) { return o.x + o.y + o.z; }
void main() {
    P p = mk(0x11223344L, 0x55667788L);
    R(0) = (unsigned char)p.a; R(1) = (unsigned char)(p.a >> 24); R(2) = (unsigned char)p.b; R(3) = (unsigned char)(p.b >> 24);
    T3 t = mk3(7); R(4) = t.a; R(5) = t.b; R(6) = t.c;            /* declaration */
    g1 = t; g2 = g1; R(7) = g2.a; R(8) = g2.b; R(9) = g2.c;       /* assignment */
    struct Point q = gp(10, 20); R(10) = q.x; R(11) = q.y;          /* 2-byte struct */
    struct V a = mkv(1), b = mkv(10);
    struct V c = a + b; R(12) = c.x; R(13) = c.y; R(14) = c.z;      /* operator, by-copy arg */
    struct V d = a.twice(); R(15) = d.x; R(16) = d.z;               /* method */
    struct V (*pf)(int) = mkv;
    struct V e = pf(5); R(17) = e.x; R(18) = e.z;                   /* function pointer */
    R(19) = sumv(a); R(20) = mkv(2).y + 4;                          /* by-copy arg, member of a result */
    R(21) = 0xAA;
}
