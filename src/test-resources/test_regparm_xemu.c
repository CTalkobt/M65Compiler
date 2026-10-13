/* test_regparm_xemu.c — __regparm: the first argument in A (char) or A:X
 * (int, pointer), the others by the normal convention
 * (linked with test_regparm_lib_xemu.c)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] one char parameter; char + char
 *   [1] int + int; char + int (the register argument must survive the
 *       loads of the others, in the caller and in the callee)
 *   [2] a pointer first parameter; three parameters
 *   [3] a caller with a stack frame (recursive)
 *   [4] calls into another module (the callee's normal entry)
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

__attribute__((noinline)) __regparm unsigned char twice(unsigned char c) { return c + c; }
__attribute__((noinline)) __regparm unsigned char addc(unsigned char c, unsigned char d) { return c + d; }
__attribute__((noinline)) __regparm int addi(int a, int b) { return a + b; }
__attribute__((noinline)) __regparm int mixed(unsigned char c, int b) { return c + b; }
__attribute__((noinline)) __regparm int first(const char *s, int i) { return s[i]; }
__attribute__((noinline)) __regparm int three(int a, unsigned char b, int c) { return a - b + c; }

__regparm unsigned char x_addc(unsigned char c, unsigned char d);
__regparm int x_addi(int a, int b, int c);

static int rec(int n) {
    if (n == 0) return addi(n + 100, 23);
    return rec(n - 1) + mixed((unsigned char)n, 1);
}

int main(void) {
    volatile unsigned char v = 21;
    volatile int w = 1000;
    static const char str[] = "abc";
    OUT[0] = twice(v) == 42 && addc(v, 3) == 24 && addc(3, v) == 24;
    OUT[1] = addi(w, 234) == 1234 && addi(234, w) == 1234 && mixed(v, w) == 1021;
    OUT[2] = first(str, 1) == str[1] && three(w, v, 7) == 986;
    OUT[3] = rec(3) == 123 + 2 + 3 + 4;
    OUT[4] = x_addc(v, 4) == 25 && x_addi(w, 30, 6) == 1024;
    OUT[5] = 0xAA;
    return 0;
}
