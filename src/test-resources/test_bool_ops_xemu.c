/* test_bool_ops_xemu.c — _Bool in every position (the audit after the
 * _Bool fixes): results at $C0C0..$C0D7 and $C0D8..$C0E3, all 01
 *
 *   $C0C0: promotion (a + a, -a, ~a), comparisons into _Bool, switch, _Bool
 *   bit-fields, va_arg promotion, from a float, ++ (always 1), -- (turns
 *   it over), *=, <<=, >>=, array elements, through a pointer, a return
 *   from a long and an inline function, a parameter through a function
 *   pointer ([20]), ?:, sizeof/_Alignof, <stdbool.h>, to int, a static
 *   initializer, from a long, !
 *   $C0D8: ++ of a struct member (1 byte), -- of an array element, from a
 *   long long ([2], cast [10]), == 2 is false, a union, a _Bool function in
 *   a loop, a global initializer, the values of b++ / ++b / b--, (*q)++
 */
#include <stdarg.h>
#include <stdbool.h>
#define OUT ((volatile unsigned char *)0xC0C0)
struct S { _Bool f : 1; _Bool g : 1; unsigned char x; } s;
_Bool garr[3];
static int sum_va(int n, ...) { va_list ap; int t = 0; va_start(ap, n); while (n--) t += va_arg(ap, int); va_end(ap); return t; }
static void take(_Bool v) { OUT[20] = v; }
static _Bool ret_long(long v) { return v; }
static inline _Bool inl(int v) { return v; }
static void part2(void);
int main(void) {
    _Bool a = 1, b = 0, c;
    int i; long l = 0x10000L; float f0 = 0.0f; unsigned char *p = 0; _Bool *bp = &c;
    void (*fp)(_Bool) = take;
    unsigned char k = 0;
    OUT[0] = (a + a) == 2;                 /* promotion to int */
    OUT[1] = (-a) == -1 && (~a) == -2;
    c = a < b; OUT[2] = c == 0;
    c = (p == 0); OUT[3] = c == 1;
    switch (a) { case 1: k = 1; break; default: k = 2; }
    OUT[4] = k == 1;
    s.f = 2; s.g = 0; s.x = 0x77;
    OUT[5] = s.f == 1 && s.g == 0 && s.x == 0x77;
    OUT[6] = sum_va(3, a, b, (_Bool)5) == 2;
    c = f0; OUT[7] = c == 0;
    c = 0; c++; c++; OUT[8] = c == 1;
    c = 0; c--; OUT[9] = c == 1;           /* 0 - 1 = -1: true */
    c = 1; c--; OUT[10] = c == 0;
    c = 1; c *= 2; OUT[11] = c == 1;
    c = 1; c <<= 1; OUT[12] = c == 1;
    c = 1; c >>= 1; OUT[13] = c == 0;
    garr[1] = 300; OUT[14] = garr[1] == 1 && garr[0] == 0 && garr[2] == 0;
    *bp = 5; OUT[15] = c == 1;
    OUT[16] = ret_long(l) == 1 && inl(256) == 1;
    fp(512);
    OUT[17] = (a ? 7 : 9) == 7 && sizeof(_Bool) == 1 && _Alignof(_Bool) == 1;
    { bool t = true, u = false; OUT[18] = t && !u && true == 1; }
    i = a; OUT[19] = i == 1;
    { static _Bool st = 42; OUT[21] = st == 1; }
    c = l; OUT[22] = c == 1;
    c = (unsigned char)0; c = !c; OUT[23] = c == 1;
    part2();
    return 0;
}

#define OUT2 ((volatile unsigned char *)0xC0D8)
struct T { _Bool m; unsigned char after; } t = { 0, 0x66 };
_Bool arr2[3];
_Bool gaddr = 5;
union U { _Bool b; unsigned char c; } u;
static _Bool is_odd(unsigned v) { return v & 1; }
static void part2(void) {
    _Bool c; unsigned char i = 1, n = 0;
    long long ll = 0x100000000LL;
    t.m++; t.m++;
    OUT2[0] = t.m == 1 && t.after == 0x66;
    arr2[i]--;                       /* 0 -> 1 */
    OUT2[1] = arr2[1] == 1 && arr2[0] == 0 && arr2[2] == 0;
    c = ll; OUT2[2] = c == 1;
    c = 1; OUT2[3] = !(c == 2) && c != 2;
    u.c = 0; u.b = 9; OUT2[4] = u.c == 1;
    for (i = 0; i < 6; i++) if (is_odd(i)) n++;
    OUT2[5] = n == 3;
    OUT2[6] = gaddr == 1;
    { _Bool b2 = 0; _Bool r = b2++; OUT2[7] = r == 0 && b2 == 1; r = ++b2; OUT2[8] = r == 1; r = b2--; OUT2[9] = r == 1 && b2 == 0; }
    { _Bool w = (_Bool)ll; OUT2[10] = w; }
    { unsigned char k = 0; _Bool *q = &arr2[2]; (*q)++; k = arr2[2]; OUT2[11] = k == 1; }
}
