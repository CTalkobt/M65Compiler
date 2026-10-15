/* test_methods_nested_xemu.c — struct methods and nested functions (bugs
 * found writing the rogue, forth and calc_graph examples)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a method called on a global object gets its address as this (it
 *       got the object's contents: g.set(9) wrote elsewhere)
 *   [1] a method whose return type is unsigned char / signed int (a parse
 *       error); a method of the base struct called on a derived one
 *   [2] a function-like macro without parameters, F() (never expanded)
 *   [3] a nested function returning unsigned char / unsigned long (a parse
 *       error)
 *   [4] a nested function reading through the enclosing function's char
 *       pointer (loaded 16 bits) and storing into its array (it treated the
 *       array as a pointer)
 *   [5] array designators: an enum constant, (0), 1 + 1 (a parse error); an
 *       unsized array is as long as its highest designated element (it was
 *       its number of initializers: { [2] = 5, [0] = 1 } had 2)
 *   [6] a recursive nested function (its static link shared a frame slot
 *       with its parameter: it never ended); a forward-declared one
 *       (int h(int); or auto int h(int); was an undefined external)
 *   [7] shadowing: an inner block's, a for loop's and a statement
 *       expression's variable hide the outer one only in their scope
 *   [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct T {
    unsigned char x, y;
    int get(void) { return this->x; }
    void set(int v) { this->x = v; }
    unsigned char ux(void) { return this->x; }
    signed int neg(void) { return -(signed int)this->x; }
};
struct T g;
struct T arr[3];
struct D : T { unsigned char z; };
struct D dg;

enum { EA, EB };
static const int des1[] = { [EB] = 5, [EA] = 1 };
static const int des2[] = { [1 + 1] = 5, [(0)] = 1 };

static int rec_sum(int n) { int k = 3; int g(int a) { return a ? k + g(a - 1) : 0; } return g(n); }
static int fwd(int n) {
    int k = 3;
    int h(int a);
    auto int h2(int a);
    int g(int a) { return h(a) + h2(a); }
    int h(int a) { return a + k; }
    int h2(int a) { return a * k; }
    return g(n);
}

static signed int shadow_for(signed int a) {
    signed int r = 0;
    for (signed int a = 1; a <= 3; a++) r += a;
    return r * 100 + a;
}

#define FIVE() 5
#define NOTHING()

static int nested_ret(int a) {
    unsigned char inc(void) { return a + 1; }
    unsigned long big(void) { return 70000UL + a; }
    return inc() + (big() == 70002UL);
}

static const char words[8] = { 'a', 'b', ' ', 'c', 0, 'x', 'y', 0 };
static unsigned char count_words(const char *line) {
    char tok[8];
    unsigned char k = 0, last = 0;
    unsigned char word(void) {
        const char *p = line;
        unsigned char n = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ') { char c = *p++; tok[n++] = c; }
        tok[n] = 0;
        line = p;
        return n;
    }
    while (word()) { k++; last = tok[0]; }
    return k == 2 && last == 'c';
}

int main(void) {
    g.x = 3;
    g.set(9);
    arr[1].set(4);
    OUT[0] = g.x == 9 && g.get() == 9 && arr[1].get() == 4;
    dg.x = 7;
    OUT[1] = g.ux() == 9 && g.neg() == -9 && dg.ux() == 7 && dg.get() == 7;
    OUT[2] = FIVE() NOTHING() == 5;
    OUT[3] = nested_ret(2) == 4;
    OUT[4] = count_words(words);
    OUT[5] = sizeof des1 == 4 && des1[1] == 5 && sizeof des2 == 6 && des2[0] == 1 && des2[2] == 5;
    OUT[6] = rec_sum(4) == 12 && fwd(4) == 4 + 3 + 12;
    {
        signed int a = 5, x, z = 0;
        { signed int a = 7; x = a; { unsigned char a = 9; z = a; } x += a; }
        OUT[7] = x == 14 && a == 5 && z == 9 && shadow_for(4) == 604 &&
                 ({ signed int _a = 5; ({ signed int _a = 7; _a; }) + _a; }) == 12;
    }
    OUT[8] = 0xAA;
    return 0;
}
