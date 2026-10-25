/* test_pp_expand.c — macro expansion as C11 6.10.3 has it (test_compiler.sh):
 * arguments expanded before substitution, a macro not expanding itself again
 * in its replacement (hide sets), # and ## on raw arguments, placemarkers,
 * GNU , ## __VA_ARGS__. test_pp_expand.expected: what gcc -E -P gives */
#define STR(x) #x
#define XSTR(x) STR(x)
#define CAT(a,b) a##b
#define XCAT(a,b) CAT(a,b)
#define N 4
#define SELF SELF + 1
#define f(a) a*g
#define g(a) f(a)
#define x 3
#define h(y) y + x
#define AA BB
#define BB AA
a1: XSTR(N)
a2: XCAT(v,N)
a3: SELF
a4: f(2)(9)
a5: h(x)
a6: AA BB
a7: STR(N)
a8: CAT(v,N)
a9: XSTR(XCAT(a,b))
#define DO_PRAGMA(x) _Pragma(#x)
#define LINE_STR XSTR(__LINE__)
#define VA(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define VB(...) gg(0, __VA_ARGS__)
#define MINUS -
#define EMPTY
#define PASTE3(a,b,c) a##b##c
a10: LINE_STR
a11: VA("x") VA("y", 1, 2)
a12: VB(1, (2, 3))
a13: -MINUS 1
a14: PASTE3(x, , z) CAT(, y) CAT(x, )
a15: STR(  a   +  "b\n" )
a16: XSTR(EMPTY)
