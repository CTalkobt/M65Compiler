// #657: in a function with static storage a member of a local struct (an
// element of a local array at a constant index, a local whose address was
// taken) is read and written at its fixed address, not through a pointer
// built from &local. What must hold: a value written through the address by
// someone else is what the direct read sees, and the other way round.
// Results at $C0C0: 01 per check, $AA sentinel.
struct line { unsigned start, y; unsigned char h, asc; };
struct wide { long l; float f; unsigned char tag; };
struct in2 { unsigned char a; unsigned b; };
struct outer { unsigned char pad; struct in2 in; unsigned tail; };
struct bits { unsigned lo : 3; unsigned mid : 5; unsigned char after; };

void ld_get_line(unsigned l, struct line *L);
unsigned ld_sum_line(const struct line *L);
void ld_poke(unsigned *p, unsigned v);
unsigned ld_peek(const unsigned *p);
void ld_poke8(unsigned char *p, unsigned char v);
void ld_wide(struct wide *w);
long ld_wide_l(const struct wide *w);
void ld_outer(struct outer *o);
unsigned ld_outer_sum(const struct outer *o);
void ld_bits(struct bits *b);
void ld_inc(unsigned *p);

volatile unsigned vz = 0;

// the issue's loop: the struct filled through its address, its members read
static unsigned find(unsigned n, unsigned dy) {
    unsigned lo = 0, hi = n, mid;
    struct line L;
    while (lo < hi) {
        mid = (lo + hi) / 2;
        ld_get_line(mid, &L);
        if (L.y + L.h <= dy) lo = mid + 1; else hi = mid;
    }
    return lo;
}
static unsigned char t_find(void) { return find(40, 121) == 11 && find(40, 100) == 0 && find(40, 999) == 40; }

// members written here, read through the address there
static unsigned char t_write(void) {
    struct line L;
    L.start = 1000; L.y = 200; L.h = 30; L.asc = 4;
    unsigned a = ld_sum_line(&L);
    L.y = 201; L.h = 31;
    return a == 1234 && ld_sum_line(&L) == 1236;
}

// a write through a pointer kept in a variable between two direct reads
static unsigned char t_alias(void) {
    struct line L;
    struct line *volatile q = &L;
    L.y = 1;
    unsigned a = L.y;
    q->y = 2;
    unsigned b = L.y;
    L.y = 3;
    unsigned c = q->y;
    q->h = 9;
    return a == 1 && b == 2 && c == 3 && L.h == 9;
}

// each call may have changed the member: read again after each
static unsigned char t_calls(void) {
    struct line L;
    L.y = 5;
    ld_poke(&L.y, 6);
    unsigned a = L.y;
    ld_inc(&L.y);
    unsigned b = L.y;
    ld_inc(&L.y);
    return a == 6 && b == 7 && L.y == 8 && ld_peek(&L.y) == 8;
}

// a local array at constant indexes, and at a variable one
static unsigned char t_array(void) {
    unsigned a[4];
    unsigned char c[5];
    a[0] = 10; a[3] = 13;
    ld_poke(&a[3], 99);
    ld_poke(a, 50);
    c[4] = 1;
    ld_poke8(c + 4, 77);
    unsigned i = vz + 3;
    return a[3] == 99 && a[0] == 50 && c[4] == 77 && a[i] == 99;
}

// long and float members
static unsigned char t_wide(void) {
    struct wide w;
    w.l = 1; w.f = 1.0f; w.tag = 1;
    ld_wide(&w);
    unsigned char ok = w.l == 100000L && w.f == 2.5f && w.tag == 9;
    w.l = 70000L;
    return ok && ld_wide_l(&w) == 70000L;
}

// a struct in a struct, structs in an array
static unsigned char t_nested(void) {
    struct outer o;
    struct in2 arr[3];
    o.in.b = 1;
    ld_outer(&o);
    arr[2].b = 321; arr[2].a = 3;
    unsigned char ok = o.in.a == 4 && o.in.b == 500 && o.tail == 6;
    o.in.b = 501;
    return ok && ld_outer_sum(&o) == 511 && arr[2].b == 321 && arr[2].a == 3;
}

// bit-fields of a local struct
static unsigned char t_bits(void) {
    struct bits b;
    b.lo = 1; b.mid = 2; b.after = 3;
    ld_bits(&b);
    unsigned char ok = b.lo == 5 && b.mid == 17 && b.after == 33;
    b.mid = 30;
    return ok && b.lo == 5 && b.mid == 30 && b.after == 33;
}

// a scalar local whose address was taken
static unsigned char t_scalar(void) {
    unsigned v = 1;
    ld_poke(&v, 44);
    unsigned a = v;
    ld_inc(&v);
    return a == 44 && v == 45;
}

// a struct copied, then both used
static unsigned char t_copy(void) {
    struct line a, b;
    ld_get_line(3, &a);
    b = a;
    b.y = 9;
    return a.y == 103 && b.y == 9 && b.start == 30 && ld_sum_line(&b) == 30 + 9 + 4 + 7;
}

// a function with a stack frame (recursive): as before
static unsigned depth(unsigned n) {
    struct line L;
    ld_get_line(n, &L);
    if (n == 0) return L.y;
    return L.h + depth(n - 1);
}
static unsigned char t_recurse(void) { return depth(3) == 100 + 2 + 3 + 4; }

#pragma cc45 recurse
static unsigned char t_frame(void) {
    struct line L;
    L.y = 1;
    ld_poke(&L.y, 60);
    return L.y == 60 && L.start != 60000u - vz;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_find();
    o[1] = t_write();
    o[2] = t_alias();
    o[3] = t_calls();
    o[4] = t_array();
    o[5] = t_wide();
    o[6] = t_nested();
    o[7] = t_bits();
    o[8] = t_scalar();
    o[9] = t_copy();
    o[10] = t_recurse();
    o[11] = t_frame();
    o[12] = 0xAA;
    return 0;
}
