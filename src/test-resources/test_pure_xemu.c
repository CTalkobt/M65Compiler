/* test_pure_xemu.c — __pure / __attribute__((pure)) and
 * __attribute__((const)) functions (Issue #76): repeated calls share one
 * result, an unused call is dropped, a call is hoisted out of a loop
 *
 * The functions count their calls in a global (which breaks their promise,
 * on purpose: the count shows which calls the optimizer kept).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] const: sq(a) + sq(a) calls sq once; the result is right
 *   [1] pure: len(s) twice with a store to a global between: two calls (the
 *       store may change what len reads), one without
 *   [2] a call whose result is unused, and a void one: dropped
 *   [3] a const call in a loop condition: hoisted, called once for 5 rounds
 *   [4] a const call in a conditional block of a loop: not hoisted (called
 *       only when the condition holds); a loop that runs no times: once
 *   [5] a pure call in a loop that stores to memory: not hoisted
 *   [6] the attribute on the prototype only; __attribute__((__const__));
 *       a struct-returning pure function (no effect: it writes its result
 *       through the hidden __sret pointer) still runs
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

unsigned int calls;
unsigned char g = 3;
unsigned char sink[8];

NOINLINE int sq(int x) __attribute__((const));
NOINLINE int sq(int x) { calls++; return x * x; }

/* reads the global g: pure, not const */
NOINLINE __pure unsigned int len(const char *s);
NOINLINE unsigned int len(const char *s) { unsigned int n = g; calls++; while (*s++) n++; return n; }

NOINLINE void touch(void) __attribute__((pure));
NOINLINE void touch(void) { calls++; }

NOINLINE int cube(int x) __attribute__((__const__));
NOINLINE int cube(int x) { calls++; return x * x * x; }

struct pair { int a, b; };
NOINLINE struct pair mk(int x) __attribute__((pure));
NOINLINE struct pair mk(int x) { struct pair p; calls++; p.a = x; p.b = x + 1; return p; }

NOINLINE static int twice(int a) { return sq(a) + sq(a); }

NOINLINE static unsigned int with_store(const char *s) {
    unsigned int n = len(s);
    g = 10;
    return n + len(s);
}

NOINLINE static unsigned int without_store(const char *s) {
    unsigned int n = len(s);
    return n + len(s);
}

NOINLINE static void unused(int a) { sq(a); touch(); }

NOINLINE static int loop_cond(int a) {
    int t = 0, i;
    for (i = 0; i < sq(a); i++) t += i;
    return t;
}

NOINLINE static int loop_if(int a, int n) {
    int t = 0, i;
    for (i = 0; i < n; i++)
        if (i == 3) t += sq(a);
    return t;
}

NOINLINE static unsigned int loop_store(const char *s, unsigned char n) {
    unsigned int t = 0;
    unsigned char i;
    for (i = 0; i < n; i++) { t += len(s); sink[i] = (unsigned char)t; }
    return t;
}

int main(void) {
    unsigned char ok;

    calls = 0;
    OUT[0] = twice(7) == 98 && calls == 1;

    g = 3; calls = 0;
    ok = with_store("abcd") == (3 + 4) + (10 + 4) && calls == 2;
    g = 3; calls = 0;
    ok = ok && without_store("abcd") == 14 && calls == 1;
    OUT[1] = ok;

    calls = 0;
    unused(5);
    OUT[2] = calls == 0;

    /* sq(2) = 4 rounds: 0 + 1 + 2 + 3 */
    calls = 0;
    OUT[3] = loop_cond(2) == 6 && calls == 1;

    calls = 0;
    ok = loop_if(5, 6) == 25 && calls == 1;
    calls = 0;
    ok = ok && loop_if(5, 2) == 0 && calls == 0;
    calls = 0;
    ok = ok && loop_cond(0) == 0 && calls == 1;
    OUT[4] = ok;

    g = 1; calls = 0;
    OUT[5] = loop_store("ab", 3) == 9 && calls == 3;

    calls = 0;
    ok = cube(3) + cube(3) == 54 && calls == 1;
    {
        struct pair p = mk(4);
        ok = ok && p.a == 4 && p.b == 5 && calls == 2;
    }
    OUT[6] = ok;

    OUT[7] = 0xAA;
    return 0;
}
