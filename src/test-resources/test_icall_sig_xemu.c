/* test_icall_sig_xemu.c — calls through function pointers and functions'
 * base-page blocks
 *
 * ln45 takes a call through a prototyped pointer to reach only the
 * functions whose address is used with the same parameter slots (cc45
 * cc45.isig.<f>.<sig> / cc45.fsig.<g>.<sig>); through an unprototyped
 * pointer, a cast or inline asm, every one of them. Each caller here keeps
 * pointers in its base-page block across a call through a pointer to a
 * function that has a block of its own: the two must not share bytes.
 * Built twice: alone (only prototyped calls, so a call ln45 missed would
 * share bytes) and with -DANY_CALLS (an unprototyped pointer and a cast,
 * which make every function whose address is used reachable; [1] and [2]
 * pass trivially without it). test_xemu.sh also checks (ln45 -vv) that
 * other_sig, whose address is used but whose parameters no pointer here
 * has, shares the bytes of caller_typed in the first build.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] through a prototyped pointer (the same slots)
 *   [1] through an unprototyped pointer, int (*)(), to a function with two
 *       parameters
 *   [2] through a pointer of another type, cast at the call
 *   [3] through a variadic pointer
 *   [4] $AA sentinel
 */
#include <stdarg.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

unsigned char buf[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

N static unsigned char twice(unsigned char x) { return (unsigned char)(x * 2); }

/* the callees: pointers kept across a call (a block of their own), written */
N unsigned char cb_typed(unsigned char *a) {
    unsigned char *p = a + 3, *q = a + 4;
    unsigned char r = twice(*p);
    return (unsigned char)(r + *q);                 /* 2*4 + 5 = 13 */
}
#ifdef ANY_CALLS
N unsigned char cb_two(unsigned char *a, unsigned char n) {
    unsigned char *p = a + n, *q = a + n + 1;
    unsigned char r = twice(*p);
    return (unsigned char)(r + *q);                 /* n = 5: 2*6 + 7 = 19 */
}
N unsigned char cb_cast(unsigned char *a) {
    unsigned char *p = a + 7, *q = a + 8;
    unsigned char r = twice(*p);
    return (unsigned char)(r + *q);                 /* 2*8 + 9 = 25 */
}
#endif
N unsigned char cb_var(unsigned char *a, ...) {
    va_list ap;
    unsigned char *p, *q;
    va_start(ap, a);
    p = a + va_arg(ap, int);
    va_end(ap);
    q = p + 1;
    {
        unsigned char r = twice(*p);
        return (unsigned char)(r + *q);             /* 9: 2*10 + 11 = 31 */
    }
}
/* address used, parameters no pointer here has: may share with callers */
N unsigned char other_sig(long v) {
    unsigned char *p = buf + (unsigned char)v, *q = p + 1;
    unsigned char r = twice(*p);
    return (unsigned char)(r + *q);
}

unsigned char (*fp_typed)(unsigned char *) = cb_typed;
#ifdef ANY_CALLS
unsigned char (*fp_any)() = cb_two;
unsigned int (*fp_other)(long) = (unsigned int (*)(long))cb_cast;
#endif
unsigned char (*fp_var)(unsigned char *, ...) = cb_var;
unsigned char (*fp_long)(long) = other_sig;

/* the callers: pointers in their blocks across the call */
N unsigned char caller_typed(void) {
    unsigned char *s = buf, *t = buf + 1;
    unsigned char r = fp_typed(buf);
    return r == 13 && *s == 1 && *t == 2;
}
#ifdef ANY_CALLS
N unsigned char caller_any(void) {
    unsigned char *s = buf + 2, *t = buf + 3;
    unsigned char r = fp_any(buf, 5);
    return r == 19 && *s == 3 && *t == 4;
}
N unsigned char caller_cast(void) {
    unsigned char *s = buf + 4, *t = buf + 5;
    unsigned char r = ((unsigned char (*)(unsigned char *))fp_other)(buf);
    return r == 25 && *s == 5 && *t == 6;
}
#else
static unsigned char caller_any(void) { return 1; }
static unsigned char caller_cast(void) { return 1; }
#endif
N unsigned char caller_var(void) {
    unsigned char *s = buf + 6, *t = buf + 7;
    unsigned char r = fp_var(buf, 9);
    return r == 31 && *s == 7 && *t == 8;
}

int main(void) {
    OUT[0] = caller_typed();
    OUT[1] = caller_any();
    OUT[2] = caller_cast();
    OUT[3] = caller_var();
    OUT[4] = 0xAA;
    return fp_long(0) == 4 ? 0 : 1;     /* 2*1 + 2 */
}
