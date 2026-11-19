// #659: a member of a struct read again with nothing between that can
// change it is the value read before (a local struct now as a global one);
// what may change it makes it read again: a store into the struct or the
// member, a struct assignment, a store through a pointer, a call when its
// address is known elsewhere or it is global. And across an if: what was
// read before the if is still valid after it unless a path through it
// changed the struct (a join block's own later call does not undo it).
// Results at $C0C0: 01 per check, $AA sentinel.
struct line { unsigned start, y; unsigned char h, asc; };
extern struct line G;
void ll_get(unsigned l, struct line *L);
void ll_bump(void);          // changes the struct last passed to ll_get
void ll_touch_g(void);       // changes G
void ll_nothing(void);

volatile unsigned char vz = 0;

// a call after the address was passed on: read again
static unsigned char t_escape(void) {
    struct line L;
    ll_get(5, &L);
    unsigned a = L.y;
    ll_bump();
    unsigned b = L.y;
    unsigned char h = L.h;
    return a == 6 && b == 16 && h == 8;
}

// a store into the member, and into another member
static unsigned char t_store(void) {
    struct line L;
    ll_get(1, &L);
    unsigned a = L.y;
    L.y = a + 40;
    unsigned b = L.y;
    L.h = 77;
    unsigned c = L.y;
    return a == 2 && b == 42 && c == 42 && L.h == 77;
}

// a whole struct assigned
static unsigned char t_assign(void) {
    struct line A, B;
    ll_get(3, &A);
    ll_get(9, &B);
    unsigned a = B.y;
    B = A;
    unsigned b = B.y;
    return a == 10 && b == 4;
}

// a store through a pointer that may point to the struct
static unsigned char t_pointer(void) {
    struct line L, M;
    ll_get(2, &L);
    ll_get(4, &M);
    struct line *p = vz ? &M : &L;
    unsigned a = L.y;
    p->y = 99;
    unsigned b = L.y;
    return a == 3 && b == 99 && M.y == 5;
}

// across an if: changed on one path (by a store, by a call)
static unsigned char t_join(void) {
    struct line L;
    ll_get(7, &L);
    unsigned a = L.y;
    if (vz == 0) L.y = 1;
    unsigned b = L.y;
    ll_get(7, &L);
    a += L.y;
    if (vz == 0) ll_bump();
    unsigned c = L.y;
    ll_nothing();
    return a == 16 && b == 1 && c == 18;
}

// across an if that changes nothing, with a call later in the join block
static unsigned char t_join_call(void) {
    struct line L;
    ll_get(20, &L);
    unsigned t = 0;
    if (L.y > 5) t = L.y - 5;
    t += L.y + L.h;
    ll_nothing();
    return t == 16 + 21 + 22;
}

// a global: a call may change it, a store into another global not
static unsigned char t_global(void) {
    G.y = 1;
    unsigned a = G.y;
    ll_touch_g();
    unsigned b = G.y;
    unsigned c = 0;
    if (vz == 0) ll_touch_g();
    c = G.y;
    return a == 1 && b == 101 && c == 201;
}

// in a loop: what the body changes is read again on each pass
static unsigned char t_loop(void) {
    struct line L;
    unsigned char i;
    unsigned s = 0;
    ll_get(0, &L);
    for (i = 0; i < 3; i++) { s += L.y; ll_bump(); }
    G.y = 0;
    for (i = 0; i < 3; i++) { s += G.y; ll_touch_g(); }
    return s == 1 + 11 + 21 + 0 + 100 + 200;
}

// volatile: each read a read (the value the same here; the code must still run)
static unsigned char t_volatile(void) {
    volatile struct line V;
    V.y = 3;
    unsigned a = V.y + V.y;
    return a == 6;
}

int main(void) {
    unsigned char *o = (unsigned char *)0xC0C0;
    o[0] = t_escape();
    o[1] = t_store();
    o[2] = t_assign();
    o[3] = t_pointer();
    o[4] = t_join();
    o[5] = t_join_call();
    o[6] = t_global();
    o[7] = t_loop();
    o[8] = t_volatile();
    o[9] = 0xAA;
    return 0;
}
