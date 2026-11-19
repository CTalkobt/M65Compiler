// #660: the address of a member of a global struct or union used as a value
// (passed to a function, assigned to a pointer: &g.m, g.arr, &g.arr[k]) is
// the constant g+offset (ldax #_area+100), not the global's address and an
// add at run time. The addresses must be the members' own, and writes
// through them land there.
// Results at $C0C0: 01 per check, $AA sentinel.
#include <string.h>
union area { unsigned char m[256]; struct { unsigned char pad[100]; unsigned char rows[50]; unsigned w[20]; } r; };
extern union area area;
unsigned ma_addr(const void *p);
void ma_fill(unsigned char *p, unsigned char v, unsigned char n);
unsigned char ma_sum(const unsigned char *p, unsigned char n);
void ma_putw(unsigned *p, unsigned v);

struct item { unsigned char a; unsigned b; };
struct shelf { unsigned char tag; struct item items[10]; unsigned char after; };
static struct shelf sh;
struct inner { unsigned char x[8]; };
struct outer { unsigned char p; struct inner in[4]; };
static struct outer ou;
__zp struct { unsigned char lo; unsigned char v[6]; } zs;

static unsigned char t_addresses(void) {
    unsigned base = ma_addr(&area);
    return ma_addr(area.r.rows) == base + 100 && ma_addr(&area.r.rows[3]) == base + 103 &&
           ma_addr(&area.r.w[2]) == base + 154 && ma_addr(&area.r.w) == base + 150 &&
           ma_addr(&sh.items[2].b) == ma_addr(&sh) + 1 + 2 * 3 + 1 && ma_addr(&sh.after) == ma_addr(&sh) + 31 &&
           ma_addr(ou.in[2].x) == ma_addr(&ou) + 1 + 16 && ma_addr(&zs.v[4]) == ma_addr(&zs) + 5;
}

static unsigned char t_through(void) {
    unsigned i;
    for (i = 0; i < 256; i++) area.m[i] = 0;
    ma_fill(area.r.rows, 7, 50);
    ma_putw(&area.r.w[1], 0x1234);
    memset(&area.r.rows[10], 1, 5);
    return area.m[99] == 0 && area.m[100] == 7 && area.m[149] == 7 && area.m[150] == 0 &&
           area.m[110] == 1 && area.m[114] == 1 && area.m[115] == 7 &&
           area.m[152] == 0x34 && area.m[153] == 0x12 && ma_sum(area.r.rows, 50) == (unsigned char)(45 * 7 + 5);
}

static unsigned char t_pointer(void) {
    unsigned char *p = area.r.rows + 5;
    unsigned *q = &area.r.w[19];
    struct item *it = &sh.items[9];
    *p = 77; *q = 999; it->b = 4321;
    unsigned char *z = &zs.v[1];
    *z = 66;
    return area.m[105] == 77 && area.r.w[19] == 999 && sh.items[9].b == 4321 && zs.v[1] == 66;
}

// a pointer kept and stepped from a member
static unsigned char t_walk(void) {
    unsigned char *p = ou.in[1].x;
    unsigned char i, s = 0;
    for (i = 0; i < 8; i++) ou.in[1].x[i] = i + 1;
    for (i = 0; i < 8; i++) s += *p++;
    return s == 36 && p == ou.in[2].x;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_addresses();
    o[1] = t_through();
    o[2] = t_pointer();
    o[3] = t_walk();
    o[4] = 0xAA;
    return 0;
}
