/* test_global_addr_init_xemu.c — globals initialized with link-time addresses
 *
 * Results at $C0C0:
 *   [0] *g = 5 through int *g = &b           -> 5
 *   [1] char *q = buf; q[2]                  -> 3
 *   [2] int (*fnp)(void) = seven; fnp()      -> 7
 *   [3] int *tab[2] = {&c, &b}; *tab[0]      -> 7
 *   [4] tab[1] == &b                         -> 1
 *   [5] pb == &b (pointer compare against a relocated address) -> 1
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
int b, c = 7, *g = &b;
char buf[4] = {1, 2, 3, 4};
char *q = buf;
static int seven(void) { return 7; }
int (*fnp)(void) = seven;
int *tab[2] = { &c, &b };
int *pb = &b;
int main(void) {
    *g = 5;
    OUT[0] = b;
    OUT[1] = q[2];
    OUT[2] = fnp();
    OUT[3] = *tab[0];
    OUT[4] = tab[1] == &b;
    OUT[5] = pb == &b;
    OUT[6] = 0xAA;
    return 0;
}
