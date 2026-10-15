/* test_parse_fixes_xemu.c — the comma operator in for clauses, and macro
 * arguments that contain a parameter's name
 *
 * Results at $C0C0 (01 = pass):
 *   [0] for (i = 0, j = 10; i < 5; i++, j--)
 *   [1] a comma expression as a for condition
 *   [2] F(dst, src, len) called with src = g(len): the argument's "len" is
 *       not the parameter (each parameter is substituted once)
 *   [3] SWAP(a, b) -> b a called as SWAP(y, x); a variadic argument naming
 *       the macro's parameter
 *   [4] sizeof x[i] / sizeof *p in constant expressions: an array sized
 *       sizeof t / sizeof t[0] (it got 2 bytes), 2D arrays, a pointer, an
 *       extra declarator (int a, b[n])
 *   [5] a struct member that is an array of pointers (char *d[3]) or a
 *       pointer to pointers: s->d[i] is a pointer (it loaded one byte),
 *       s->d[i][k] a char (k was scaled by 2)
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned int g(unsigned int v) { return v * 2; }
static unsigned int three(unsigned int a, unsigned int b, unsigned int c) { return a * 100 + b * 10 + c; }
#define F(dst, src, len) three((dst), (src), (len))
#define MINUS(a, b) ((a) - (b))
#define SWAP(a, b) MINUS(b, a)
#define FIRST(fmt, ...) three(fmt, __VA_ARGS__)

static const char *const names[] = { "one", "two", "three", "four", "five" };
static const unsigned int vals[] = { 1, 2, 3 };
static unsigned int by_names[sizeof names / sizeof names[0]];
static unsigned char m2[3][7];
static unsigned char row_copy[sizeof m2[1]];
static long *lp;
struct strs { unsigned char n; const char *d[3]; char **pp; };
static char s_a[] = "abcd", s_b[] = "efgh", s_c[] = "ijkl";
static char *s_list[2] = { s_b, s_c };
static struct strs st = { 3, { s_a, s_b, s_c }, s_list };
static const struct strs cst[2] = { { 1, { "pq", "rs", "tu" }, 0 }, { 2, { "vw", "xy", "z" }, 0 } };
static unsigned char lp_size[sizeof *lp];
static unsigned int pad1, by_vals[sizeof vals / sizeof vals[0]];

int main(void) {
    unsigned char i, j, n = 0;
    unsigned int len = 3, x = 9, y = 4, fmt = 1;

    for (i = 0, j = 10; i < 5; i++, j--) n += j;
    OUT[0] = n == 40 && i == 5 && j == 5;

    for (i = 0; n = n + 1, i < 3; i++) ;
    OUT[1] = n == 44;

    OUT[2] = F(1, g(len), 2) == 100 + 60 + 2;      /* g(3) = 6, not g(2) */

    OUT[3] = SWAP(y, x) == 5 && FIRST(2, fmt, 0) == 210;

    OUT[4] = sizeof by_names == 10 && sizeof row_copy == 7 && sizeof m2[1][2] == 1 &&
             sizeof lp_size == 4 && sizeof by_vals == 6 && sizeof names / sizeof names[0] == 5 && pad1 == 0;

    {
        struct strs *p = &st;
        const struct strs *q = &cst[1];
        unsigned char k = 3;
        OUT[5] = p->d[2] == s_c && p->d[2][3] == 'l' && st.d[1][k] == 'h' && p->pp[1][2] == 'k' &&
                 q->d[1][1] == 'y' && cst[0].d[2][0] == 't' && *p->d[0] == 'a';
    }
    OUT[6] = 0xAA;
    return 0;
}
