/* test_cse_slots_xemu.c — CSE keeps a variable's value across stores
 * through pointers and calls only when its address is never taken: every
 * way a variable can change behind the code's back still reloads it
 *
 * Results at $C0C0 (01 = pass):
 *   [0] &x taken: a store through the pointer changes x
 *   [1] &x kept in a global: a call changes x through it
 *   [2] a nested function changes its parent's variable
 *   [3] a local array changed through a pointer to it
 *   [4] a local struct passed by address and changed
 *   [5] a variable whose address is never taken: k * 7 the same across a
 *       store through a pointer and a call (reused), and right
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

signed int *gp;
signed int other;
struct pt { signed int a, b; };

__attribute__((noinline)) static void touch(void) { *gp = 40; }
__attribute__((noinline)) static void setpt(struct pt *p) { p->a = 9; }
__attribute__((noinline)) static void nothing(void) { other++; }

__attribute__((noinline)) static unsigned char t_addr(void) {
    signed int x = 1, a, b;
    signed int *p = &x;
    a = x * 3;
    *p = 5;
    b = x * 3;
    return a == 3 && b == 15;
}

__attribute__((noinline)) static unsigned char t_global(void) {
    signed int x = 2, a, b;
    gp = &x;
    a = x * 5;
    touch();
    b = x * 5;
    return a == 10 && b == 200;
}

__attribute__((noinline)) static unsigned char t_nested(void) {
    signed int x = 1, a, b;
    void inc(void) { x += 10; }
    a = x * 3;
    inc();
    b = x * 3;
    return a == 3 && b == 33;
}

__attribute__((noinline)) static unsigned char t_array(void) {
    unsigned char buf[4], a, b;
    unsigned char *p = buf;
    buf[0] = 1;
    a = buf[0] + 1;
    p[0] = 7;
    b = buf[0] + 1;
    return a == 2 && b == 8;
}

__attribute__((noinline)) static unsigned char t_struct(void) {
    struct pt v;
    signed int a, b;
    v.a = 1;
    a = v.a * 3;
    setpt(&v);
    b = v.a * 3;
    return a == 3 && b == 27;
}

__attribute__((noinline)) static unsigned char t_plain(signed int k) {
    signed int a, b;
    gp = &other;
    a = k * 7;
    *gp = 3;
    nothing();
    b = k * 7;
    return a == 21 && b == 21 && other == 4;
}

int main(void) {
    OUT[0] = t_addr();
    OUT[1] = t_global();
    OUT[2] = t_nested();
    OUT[3] = t_array();
    OUT[4] = t_struct();
    OUT[5] = t_plain(3);
    OUT[6] = 0xAA;
    return 0;
}
