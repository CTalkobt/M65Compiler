/* test_nested_decl_xemu.c — nested functions, declarators, preprocessor
 *
 * Results at $C0C0 (01 = pass):
 *   [0] nested function reading a parent variable that is not the parent's
 *       first local; two levels of nesting (static link passed through)
 *   [1] non-local goto from a nested function to a parent label
 *   [2] trampoline built in a statically allocated nested function
 *   [3] T (*p)[N] pointers to arrays (local and global): indexing, *p,
 *       p + 1, sizeof *p
 *   [4] __attribute__((alias)), aligned functions, __alignof__(expr)
 *   [5] "# define", named variadic macros (args...), L'x' with a macro L,
 *       a macro parameter named L
 *   [6] signed 32-bit comparison of stack-frame locals
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static int apply(int (*f)(int), int v) { return f(v); }

static int two_levels(void) {
    int pad = 7;
    unsigned x = pad - 7;
    void mid(void) {
        void inner(void) { x += 4; }
        inner();
        inner();
    }
    mid();
    return (int)x + pad;
}

static int nl_target(int n) {
    __label__ out;
    void jump(void) { goto out; }
    if (n) jump();
    return 1;
out:
    return 2;
}

static int tramp(int k) {
    int add(int v) { return v + k; }
    int via(int v) { return apply(add, v); }
    return via(10);
}

char grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
char (*gp)[3] = &grid[0];

int target_var = 5;
extern int alias_var __attribute__((alias("target_var")));
void aligned_fn(void) __attribute__((aligned(64)));
void aligned_fn(void) {}

# define TWICE(x) ((x) * 2)
#define CALL(f, args...) f(args)
#define L 99
#define WIDE(L) (L'1' + (L))

static int add3(int a, int b, int c) { return a + b + c; }

/* A stack frame (the nested function uses a local): its 32-bit locals are
   read with ldaxyz.local */
static int dist_sum(void) {
    signed long j, k, n = 0;
    int bias = 0;
    int get_bias(void) { return bias; }
    for (j = 0; j < 3; ++j)
        for (k = 0; k < 3; ++k)
            n += j > k ? j - k : k - j;
    return (int)n + get_bias();
}

int main(void) {
    OUT[0] = two_levels() == 15;
    OUT[1] = nl_target(1) == 2 && nl_target(0) == 1;
    OUT[2] = tramp(5) == 15;
    {
        char (*p)[3] = grid;
        p[1][2] = 9;
        OUT[3] = grid[1][2] == 9 && gp[1][0] == 4 && (*gp)[2] == 3 && sizeof(*p) == 3 &&
                 (char *)(p + 1) == &grid[1][0];
    }
    alias_var = 8;
    OUT[4] = target_var == 8 && &alias_var == &target_var &&
             ((unsigned)aligned_fn & 63) == 0 && __alignof__(aligned_fn) == 64;
    OUT[5] = TWICE(3) == 6 && CALL(add3, 1, 2, 3) == 6 && L'1' == '1' && WIDE(1) == '1' + 1 && L == 99;
    OUT[6] = dist_sum() == 8;
    OUT[7] = 0xAA;
    return 0;
}
