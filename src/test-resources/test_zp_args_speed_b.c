/* test_zp_args_speed_b.c — the other file of test_zp_args_speed_xemu.c (#359): static-
 * allocation functions whose parameters live across calls, so their entry
 * from another file copies them (by a list at -O2, inline at -O3) */
unsigned long acc;
__attribute__((noinline)) void add(unsigned long v) { acc += v; }
void f3(unsigned int x, unsigned int y, unsigned char c) { add(x); add(y); add(c); }
void f5(unsigned char a, unsigned long b, unsigned int c, unsigned char d, unsigned int e) {
    add(a); add(b); add(c); add(d); add(e); add(a); add(b);
}
unsigned int f1(unsigned char a) { add(a); return a * 3; }
