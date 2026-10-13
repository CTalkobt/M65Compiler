#pragma cc45 no_zp_save
/* test_stackframe_xemu.c — stack frames: recursion, nested calls with locals, long return
 *
 * Expected at $C0B0: 15 79 00 56 12 34 AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0B0 + (i)))
#pragma cc45 recurse
int fib(int n) { if (n <= 1) return n; return fib(n - 1) + fib(n - 2); }
int inner(int a) { int t = a * 3; return t + 1; }
int outer(int b) { int keep = b + 100; int r = inner(b); return keep + r; }
long lret(int x) { long v = x; v = v * 65536L + 0x1234; return v; }
void main() {
    R(0) = fib(8);            /* 21 = 0x15 */
    R(1) = outer(5);          /* 105 + 16 = 121 = 0x79 */
    long l = lret(0x56);
    R(2) = (char)(l >> 24); R(3) = (char)(l >> 16); R(4) = (char)(l >> 8); R(5) = (char)l;  /* 00 56 12 34 */
    R(6) = 0xAA;
}
