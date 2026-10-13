/* test_libcall_conv_xemu.c — calls to the library's assembly routines
 * without a declaration (no headers): __builtin_alloca, __builtin_setjmp /
 * __builtin_longjmp, __builtin_abs / __builtin_labs, and implicitly declared
 * alloca and strlen. Those routines use the ZP convention in both libraries;
 * cc45 calls them with it in either convention, so a stack-convention build
 * links without bridge thunks (test_xemu.sh links it with ln45 --no-thunks).
 * A function the unit defines itself (toupper here) keeps the unit's
 * convention. Built with -fimplicit-function-declaration.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] __builtin_alloca and implicit alloca blocks are distinct and usable
 *   [1] __builtin_abs            [2] __builtin_labs
 *   [3] implicit strlen          [4] __builtin_setjmp / __builtin_longjmp
 *   [5] the unit's own toupper is called
 *   [6] a third alloca after setjmp/longjmp keeps the same spacing (SP intact)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

void *env[5];
volatile signed int vneg = -1234;
volatile signed long lneg = -30000L;
volatile unsigned int n = 6;
int own_calls;

/* This unit's own toupper: not the library's routine */
int toupper(int c) { own_calls++; return c + 1; }

void jump_back(void) { __builtin_longjmp(env, 1); }

int main(void) {
    volatile int stage = 0;
    char *p = (char *)__builtin_alloca(n);
    char *q = (char *)alloca(n);
    char *r;
    p[0] = 1; p[5] = 2; q[0] = 3; q[5] = 4;
    OUT[0] = p[0] == 1 && p[5] == 2 && q[0] == 3 && q[5] == 4 && (q + 6 <= p || p + 6 <= q);
    OUT[1] = __builtin_abs(vneg) == 1234;
    OUT[2] = (int)__builtin_labs(lneg) == 30000;
    OUT[3] = strlen("hello") == 5;
    if (__builtin_setjmp(env) == 0) {
        stage = 1;
        jump_back();
        stage = 2;
    }
    OUT[4] = stage == 1;
    OUT[5] = toupper('a') == 'b' && own_calls == 1;
    r = (char *)__builtin_alloca(n);
    OUT[6] = (p - q) == (q - r);
    OUT[7] = 0xAA;
    return 0;
}
