/* test_regparm_lib_xemu.c — second module of test_regparm_xemu.c: __regparm
 * functions called from the other module (through their normal entry) */
__attribute__((noinline)) __regparm unsigned char x_addc(unsigned char c, unsigned char d) { return c + d; }
__attribute__((noinline)) __regparm int x_addi(int a, int b, int c) { return a + b - c; }
