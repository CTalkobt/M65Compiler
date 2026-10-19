/* test_block_extern_xemu.c — extern declarations in a block (Issue #308):
 * the file-scope name, for the block (a local of the name hidden until the
 * block ends); a function's a prototype
 *
 * Results at $C0C0 (01 = pass):
 *   [0] extern int g in a function reads the global (defined after it)
 *   [1] a local g, then an inner block's extern g: the global there, the
 *       local again after the block
 *   [2] an assignment through a block's extern changes the global
 *   [3] extern char buf[] in a block: its elements
 *   [4] extern int twice(int) in a block, then called
 *   [5] two declarators: extern int g, h
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

NOINLINE static int read_g(void) {
    extern int g;
    return g;
}

NOINLINE static int shadowed(void) {
    int g = 7, r;
    {
        extern int g;
        r = g;                  /* the global */
    }
    return r * 100 + g;         /* the local again */
}

NOINLINE static void set_g(int v) {
    extern int g;
    g = v;
}

NOINLINE static char second(void) {
    extern char buf[];
    return buf[1];
}

NOINLINE static int call(void) {
    extern int twice(int);
    return twice(21);
}

NOINLINE static int both(void) {
    extern int g, h;
    return g + h;
}

int g = 5, h = 30;
char buf[] = "xyz";
int twice(int x) { return x * 2; }

int main(void) {
    OUT[0] = read_g() == 5;
    OUT[1] = shadowed() == 507;
    set_g(9);
    OUT[2] = g == 9 && read_g() == 9;
    OUT[3] = second() == 'y';
    OUT[4] = call() == 42;
    OUT[5] = both() == 39;
    OUT[6] = 0xAA;
    return 0;
}
