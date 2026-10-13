/* test_rmw_ptr_xemu.c — read-modify-write through a pointer (*p += v,
 * p->f++, p->f -= v), done in place by cc45
 *
 * Results at $C0C0 (01 = pass):
 *   [0] char / int / long += constant through a struct pointer (carries)
 *   [1] -= constant, ++ and -- (borrows)
 *   [2] += / -= a variable
 *   [3] *p += x where p points at x itself
 *   [4] the same in a function with a stack frame
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINL __attribute__((noinline))

struct S { unsigned char c; unsigned int i; unsigned long l; };
struct S s;

NOINL void add_const(struct S *p) { p->c += 3; p->i += 0x1FF; p->l += 0x10001UL; }
NOINL void sub_inc(struct S *p) { p->c -= 5; p->i -= 0x101; p->l--; p->i++; p->c++; }
NOINL void add_var(struct S *p, unsigned int v, unsigned long w) { p->i += v; p->l -= w; p->c += (unsigned char)v; }
NOINL void self(unsigned long *p, unsigned long *x) { *p += *x; }
#pragma cc45 recurse
NOINL void frame_rmw(struct S *p, unsigned int v) { p->i += v; p->l += 7; p->c -= 1; }

int main(void) {
    unsigned long x;
    s.c = 0xFE; s.i = 0xFF02; s.l = 0x0000FFFFUL;
    add_const(&s);
    OUT[0] = s.c == 0x01 && s.i == 0x0101 && s.l == 0x00020000UL;
    sub_inc(&s);
    OUT[1] = s.c == 0xFD && s.i == 0x0001 && s.l == 0x0001FFFFUL;
    add_var(&s, 0x0102, 0x10000UL);
    OUT[2] = s.i == 0x0103 && s.l == 0x0000FFFFUL && s.c == 0xFF;
    x = 0x80000001UL;
    self(&x, &x);
    OUT[3] = x == 0x00000002UL;
    frame_rmw(&s, 0xFFFF);
    OUT[4] = s.i == 0x0102 && s.l == 0x00010006UL && s.c == 0xFE;
    OUT[5] = 0xAA;
    return 0;
}
