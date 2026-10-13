/* test_constparam_lib_xemu.c — second module of test_constparam_xemu.c */

__attribute__((noinline)) int f(int a) { return a + 1; }
int callf(void) { return f(1); }

static __attribute__((noinline)) int g(int a, int k) { return a * k; }
int use_static(void) { return g(3, 7) + g(4, 7); }

static __attribute__((noinline)) int h(int a) { return a + 1; }
int (*hp)(int) = h;
int use_ptr(void) { int r = hp(5); return h(1) == 2 && r == 6; }
