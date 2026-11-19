// The other file of test_local_direct_xemu.c (#657): what reads and writes
// the first file's locals through their addresses (not inlined: another file)
struct line { unsigned start, y; unsigned char h, asc; };
struct wide { long l; float f; unsigned char tag; };
struct in2 { unsigned char a; unsigned b; };
struct outer { unsigned char pad; struct in2 in; unsigned tail; };
struct bits { unsigned lo : 3; unsigned mid : 5; unsigned char after; };

void ld_get_line(unsigned l, struct line *L) { L->start = l * 10; L->y = l + 100; L->h = (unsigned char)(l + 1); L->asc = 7; }
unsigned ld_sum_line(const struct line *L) { return L->start + L->y + L->h + L->asc; }
void ld_poke(unsigned *p, unsigned v) { *p = v; }
unsigned ld_peek(const unsigned *p) { return *p; }
void ld_poke8(unsigned char *p, unsigned char v) { *p = v; }
void ld_wide(struct wide *w) { w->l = 100000L; w->f = 2.5f; w->tag = 9; }
long ld_wide_l(const struct wide *w) { return w->l; }
void ld_outer(struct outer *o) { o->in.a = 4; o->in.b = 500; o->tail = 6; }
unsigned ld_outer_sum(const struct outer *o) { return o->in.a + o->in.b + o->tail; }
void ld_bits(struct bits *b) { b->lo = 5; b->mid = 17; b->after = 33; }
void ld_inc(unsigned *p) { (*p)++; }
