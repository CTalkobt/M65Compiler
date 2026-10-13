/* test_gc_inputs_xemu.c -- cc45 -ffunction-sections: ln45 (--gc-inputs)
 * drops the program's own unreached functions and data too
 *
 * test_xemu.sh builds it with -ffunction-sections and checks that unused()
 * and its table are gone while everything reached stays.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a function called directly, and one only it calls
 *   [1] functions reached only through a table of pointers in data
 *   [2] a string and a variable the kept code uses
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINL __attribute__((noinline))

static const char big_table[300] = {1, 2, 3};     /* only unused() reads it */
int counter = 7;

NOINL int inner(int x) { return x * 3; }
NOINL int outer(int x) { return inner(x) + 1; }
NOINL int via_ptr_a(int x) { return x + 100; }
NOINL int via_ptr_b(int x) { return x + 200; }
int (*const table[2])(int) = {via_ptr_a, via_ptr_b};

NOINL int unused(int i) {
    int s = 0, k;
    for (k = 0; k < 300; k++) s += big_table[(k + i) % 300];
    return s;
}

int main(void) {
    const char *msg = "kept";
    OUT[0] = outer(4) == 13;
    OUT[1] = table[0](1) == 101 && table[1](1) == 201;
    OUT[2] = msg[0] == 'k' && msg[3] == 't' && counter == 7;
    OUT[3] = 0xAA;
    return 0;
}
