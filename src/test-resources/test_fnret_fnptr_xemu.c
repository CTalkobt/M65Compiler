/* test_fnret_fnptr_xemu.c — functions returning a function pointer declared
 * without a typedef, T (*f(params))(sig) (#752): prototypes, definitions,
 * calls through the result
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a prototype, the function defined after main: its parameters
 *       (int, char, long) reach it; the pointer it returns is called
 *   [1] a static definition; the result called at once, f(a)(b, c)
 *   [2] __fastcall__, const on the returned pointer, (void) parameters
 *   [3] a block-scope prototype; a null result
 *   [4] the typedef form beside it, the same pointers
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef int (*fn2_t)(int, char);

static int add(int x, char y) { return x + y; }
static int sub(int x, char y) { return x - y; }
static void (*seen)(void);
static unsigned char hits;
static void hit(void) { hits++; }

int (*pick(int which, char tag, long key))(int, char);

static int (*pick_static(unsigned char which))(int, char) { return which ? sub : add; }

void __fastcall__ (*const get_hit(void))(void) { return hit; }

fn2_t pick_td(unsigned char which) { return which ? sub : add; }

int main(void) {
    int (*p)(int, char);
    void (*local_fn(void))(void);
    OUT[5] = 0;

    p = pick(1, 'x', 0x12345678L);
    OUT[0] = p == sub && p(10, 3) == 7 && pick(0, 'x', 0x12345678L)(10, 3) == 13;

    OUT[1] = pick_static(0)(20, 5) == 25 && pick_static(1)(20, 5) == 15 && pick_static(0) == add;

    get_hit()();
    get_hit()();
    OUT[2] = hits == 2 && get_hit() == hit;

    OUT[3] = local_fn() == 0 && pick(0, 'y', 0x12345678L) == 0 && pick(1, 'x', 1L) == 0;

    OUT[4] = pick_td(1) == pick_static(1) && pick_td(0)(1, 1) == 2;
    OUT[5] = 0xAA;
    return 0;
}

int (*pick(int which, char tag, long key))(int, char) {
    if (tag != 'x' || key != 0x12345678L) return 0;
    return which ? sub : add;
}

void (*local_fn(void))(void) { return seen; }
