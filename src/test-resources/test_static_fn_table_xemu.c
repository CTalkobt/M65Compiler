/* test_static_fn_table_xemu.c — static functions named in array and struct
 * initializers without & are emitted (#326: the table referred to them, but
 * they were dropped as unused: an undefined symbol at assembly)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a const array of static functions, { f, g, h }, called by index
 *   [1] a struct of static functions (and a non-function member)
 *   [2] an array of structs, a static function in each; & and bare mixed
 *   [3] a static function reached only through another's table (its own
 *       caller is itself only in a table)
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef unsigned char (*fn)(unsigned char);

static unsigned char add1(unsigned char v) { return v + 1; }
static unsigned char dbl(unsigned char v) { return v * 2; }
static unsigned char neg(unsigned char v) { return (unsigned char)-v; }
static const fn table[3] = { add1, dbl, neg };

struct ops { fn first; unsigned char k; fn second; };
static unsigned char sq(unsigned char v) { return v * v; }
static unsigned char half(unsigned char v) { return v / 2; }
static const struct ops ops = { sq, 7, half };

struct cmd { char key; fn run; };
static unsigned char three(unsigned char v) { return v + 3; }
static unsigned char four(unsigned char v) { return v + 4; }
static const struct cmd cmds[] = { { 'a', three }, { 'b', &four }, { 'c', add1 } };

static unsigned char inner(unsigned char v) { return v ^ 0x55; }
static const fn inner_tab[1] = { inner };
static unsigned char outer(unsigned char v) { return inner_tab[0](v) + 1; }
static fn outer_tab[1] = { outer };

int main(void) {
    volatile unsigned char i = 1, v = 10;
    OUT[0] = table[0](v) == 11 && table[i](v) == 20 && table[2](v) == (unsigned char)-10;
    OUT[1] = ops.first(v) == 100 && ops.k == 7 && ops.second(v) == 5;
    OUT[2] = cmds[0].run(v) == 13 && cmds[i].run(v) == 14 && cmds[2].key == 'c' && cmds[2].run(v) == 11;
    OUT[3] = outer_tab[0](v) == (unsigned char)((10 ^ 0x55) + 1);
    OUT[4] = 0xAA;
    return 0;
}
