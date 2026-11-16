/* test_member_array_decay_xemu.c — #594: an array that is a member of a
 * struct or union is its address (it decays to a pointer to its element).
 * "s.a + n" took the address for a value of the element's type: its low
 * byte, the high byte 0, a pointer into page 0.
 *
 * Results at $C0C0: 1 = pass; [12] $AA.
 */
#include <stdint.h>

union U { struct { uint8_t a[300]; } r; uint8_t m[400]; } u;
struct S { uint8_t pad[3]; uint8_t a[300]; uint16_t w[10]; uint32_t l[4]; uint8_t g[4][5]; uint8_t *pp[3]; } s;
struct P { uint8_t x, y; };
struct T { uint8_t k; struct P pts[6]; } t;

uint8_t volatile vc;

uint8_t * __attribute__((noinline)) f1(uint8_t c) { return u.r.a + 80 + c; }
uint8_t * __attribute__((noinline)) f2(uint8_t c) { return s.a + 80 + c; }
uint8_t * __attribute__((noinline)) f3(uint8_t c) { return u.m + c; }
uint8_t * __attribute__((noinline)) f4(uint8_t c) { return &u.r.a[80 + c]; }
uint16_t * __attribute__((noinline)) fw(uint8_t c) { return s.w + c; }
uint32_t * __attribute__((noinline)) fl(uint8_t c) { return c + s.l; }
struct P * __attribute__((noinline)) fp(uint8_t c) { return t.pts + c; }
uint8_t * __attribute__((noinline)) farrow(struct S *p, uint16_t n) { return p->a + n; }
uint8_t __attribute__((noinline)) deref(uint8_t c) { return *(s.a + c); }
void __attribute__((noinline)) fill(uint8_t *p, uint8_t n, uint8_t v) { while (n--) *p++ = v; }
uint16_t __attribute__((noinline)) dist(uint8_t *p) { return (uint16_t)(p - s.a); }
uint8_t __attribute__((noinline)) inside(uint8_t *p) { return p >= s.a && p < s.a + 300; }

int main(void) {
    uint8_t *r = (uint8_t *)0xC0C0;
    uint8_t *q;
    vc = 5;
    r[0] = f1(vc) == (uint8_t *)&u + 85 && f1(vc) == f4(vc);
    r[1] = f2(vc) == (uint8_t *)&s + 3 + 85;
    r[2] = f3(200) == (uint8_t *)&u + 200;
    /* scaled by the element's size */
    r[3] = fw(vc) == &s.w[5] && (uint8_t *)fw(1) - (uint8_t *)fw(0) == 2;
    r[4] = fl(3) == &s.l[3] && (uint8_t *)fl(1) - (uint8_t *)fl(0) == 4;
    r[5] = fp(vc) == &t.pts[5] && (uint8_t *)fp(1) - (uint8_t *)fp(0) == 2;
    r[6] = farrow(&s, 299) == &s.a[299];
    /* read and written through it */
    s.a[7] = 0x5A; *(s.a + 280) = 0xA5;
    r[7] = deref(7) == 0x5A && s.a[280] == 0xA5;
    fill(s.a + 290, 4, 0x77);
    r[8] = s.a[289] == 0 && s.a[290] == 0x77 && s.a[293] == 0x77 && s.a[294] == 0;
    r[9] = dist(&s.a[260]) == 260 && inside(&s.a[299]) && !inside((uint8_t *)s.w);
    /* a row of a two-dimensional member, an array of pointers */
    q = s.g[2] + 3;
    s.pp[1] = s.a + 1;
    r[10] = q == &s.g[2][3] && *(s.pp + 1) == &s.a[1] && sizeof(s.a) == 300 && sizeof(s.w) == 20 &&
            sizeof(s.g) == 20 && sizeof(s.g[1]) == 5 && sizeof(s.pp) == 6;
    /* the array itself: assigned, indexed (as before) */
    q = u.r.a;
    r[11] = q == (uint8_t *)&u && &s.a[0] == s.a && sizeof(u.m) == 400;
    r[12] = 0xAA;
    return 0;
}
