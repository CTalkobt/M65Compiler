/* test_likely_xemu.c — __likely / __unlikely branch hints (Issue #75): the
 * branch not expected to run is placed after the function's return
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __unlikely(x): both outcomes; the value of __likely(x) is x
 *   [1] __likely with an else, __unlikely with an else, !__likely(x)
 *   [2] __builtin_expect(x, 0) / (x, 1); a non-constant hint is evaluated
 *   [3] cold code that returns, loops, breaks and continues
 *   [4] a goto into cold code, and a case label inside it
 *   [5] functions with stack frames (recursive, alloca) with cold code
 *   [6] nested hints, and a cold branch that is the function's last code
 *   [7] $AA sentinel
 */
#include <alloca.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

static unsigned char calls;

NOINLINE static void note(void) { calls++; }

NOINLINE static unsigned char unl(unsigned char x) {
    unsigned char r = 10;
    if (__unlikely(x == 0)) {
        note();
        r = 20;
    }
    return r + x;
}

NOINLINE static unsigned char lik_else(unsigned char x) {
    unsigned char r;
    if (__likely(x)) r = 1;
    else { note(); r = 2; }
    return r;
}

NOINLINE static unsigned char unl_else(unsigned char x) {
    unsigned char r;
    if (__unlikely(x > 5)) { note(); r = 3; }
    else r = 4;
    return r;
}

NOINLINE static unsigned char not_lik(unsigned char x) {
    if (!__likely(x != 7)) return 9;
    return 8;
}

static unsigned char side;

NOINLINE static unsigned char hint(void) { side++; return 0; }

NOINLINE static unsigned char expect(unsigned int x) {
    unsigned char r = 0;
    if (__builtin_expect(x == 300, 0)) r |= 1;
    if (__builtin_expect(x != 300, 1)) r |= 2;
    if (__builtin_expect(x, hint())) r |= 4;
    return r;
}

/* the sum of a[0..n) but stops at the first 0; skips 99s; 255 when a 200 */
NOINLINE static unsigned char loop(const unsigned char *a, unsigned char n) {
    unsigned char i, s = 0;
    for (i = 0; i < n; i++) {
        if (__unlikely(a[i] == 0)) break;
        if (__unlikely(a[i] == 99)) {
            note();
            continue;
        }
        if (__unlikely(a[i] == 200)) {
            unsigned char j;
            for (j = 0; j < 3; j++) note();
            return 255;
        }
        s += a[i];
    }
    return s;
}

NOINLINE static unsigned char jumps(unsigned char x) {
    unsigned char r = 0;
    if (x == 1) goto in;
    if (__unlikely(x == 2)) {
        r = 10;
    in:
        r++;
    }
    switch (x) {
    case 3:
        if (__unlikely(r == 0)) {
            r = 30;
    case 4:
            r += 2;
        }
        break;
    default:
        r += 100;
    }
    return r;
}

#pragma cc45 recurse
NOINLINE static unsigned int fact(unsigned char n) {
    unsigned char local[3];
    local[0] = n;
    if (__unlikely(n < 2)) {
        note();
        return 1;
    }
    return local[0] * fact(n - 1);
}

NOINLINE static unsigned char dyn(unsigned char n) {
    unsigned char *p = alloca(n), i, s = 0;
    for (i = 0; i < n; i++) p[i] = i;
    if (__unlikely(n > 10)) {
        note();
        s = 100;
    }
    for (i = 0; i < n; i++) s += p[i];
    return s;
}

static unsigned char last;

NOINLINE static void tail(unsigned char x) {
    last = 1;
    if (__unlikely(x)) {
        if (__likely(x < 10)) last = 2;
        else if (__unlikely(x == 50)) last = 3;
        else last = 4;
    }
}

NOINLINE static long wide(long v) {
    if (__unlikely(v < 0x10000L)) return v + 1;
    return v - 1;
}

int main(void) {
    static const unsigned char a1[] = { 1, 2, 3, 0, 9 };
    static const unsigned char a2[] = { 1, 99, 2, 99, 3 };
    static const unsigned char a3[] = { 1, 200, 3 };
    unsigned char ok, c0;

    calls = 0;
    ok = unl(5) == 15 && calls == 0 && unl(0) == 20 && calls == 1;
    ok = ok && __likely(5) == 5 && __unlikely(3 + 4) == 7;
    OUT[0] = ok;

    calls = 0;
    ok = lik_else(1) == 1 && lik_else(0) == 2 && unl_else(9) == 3 && unl_else(1) == 4 && calls == 2;
    ok = ok && not_lik(7) == 9 && not_lik(6) == 8;
    OUT[1] = ok;

    side = 0;
    OUT[2] = expect(300) == 5 && expect(5) == 6 && expect(0) == 2 && side == 3;

    calls = 0;
    ok = loop(a1, 5) == 6 && calls == 0;
    ok = ok && loop(a2, 5) == 6 && calls == 2;
    ok = ok && loop(a3, 3) == 255 && calls == 5;
    OUT[3] = ok;

    OUT[4] = jumps(0) == 100 && jumps(1) == 101 && jumps(2) == 111 &&
             jumps(3) == 32 && jumps(4) == 2;

    calls = 0;
    ok = fact(5) == 120 && calls == 1;
    c0 = calls;
    ok = ok && dyn(4) == 6 && calls == c0 && dyn(12) == 166 && calls == c0 + 1;
    OUT[5] = ok;

    tail(0); ok = last == 1;
    tail(3); ok = ok && last == 2;
    tail(50); ok = ok && last == 3;
    tail(60); ok = ok && last == 4;
    ok = ok && wide(5) == 6 && wide(0x20000L) == 0x1FFFFL;
    OUT[6] = ok;

    OUT[7] = 0xAA;
    return 0;
}
