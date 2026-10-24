/* test_enum_global_xemu.c — file-scope variables of an enum type (#337):
 * "enum colour shade;" was parsed as a function and failed ("Expected
 * '('"); a pointer or array of it failed too.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] enum colour shade; (zeroed: RED), then assigned
 *   [1] enum colour lit = BLUE; (an initializer)
 *   [2] enum colour a1, a2 = GREEN; (two declarators)
 *   [3] enum colour arr[3] = { BLUE, RED, GREEN };
 *   [4] enum colour *pp, through it
 *   [5] const enum colour fixed = GREEN;
 *   [6] a function taking and returning the enum, declared after them
 *   [7] its size: an int's (2)
 *   [8] $AA sentinel
 */
#define R ((volatile unsigned char *)0xC0C0)

enum colour { RED, GREEN, BLUE };

enum colour shade;
enum colour lit = BLUE;
enum colour a1, a2 = GREEN;
enum colour arr[3] = { BLUE, RED, GREEN };
enum colour *pp;
const enum colour fixed = GREEN;
enum colour next(enum colour c);

enum colour next(enum colour c) { return c == BLUE ? RED : (enum colour)(c + 1); }

int main(void) {
    unsigned char i;
    for (i = 0; i < 9; i++) R[i] = 0;
    R[0] = shade == RED;
    shade = BLUE;
    R[0] = R[0] && shade == BLUE;
    R[1] = lit == BLUE;
    R[2] = a1 == RED && a2 == GREEN;
    R[3] = arr[0] == BLUE && arr[1] == RED && arr[2] == GREEN;
    pp = &arr[2];
    *pp = BLUE;
    R[4] = arr[2] == BLUE && *pp == BLUE;
    R[5] = fixed == GREEN;
    R[6] = next(RED) == GREEN && next(BLUE) == RED && next(shade) == RED;
    R[7] = sizeof shade == 2 && sizeof arr == 6;
    R[8] = 0xAA;
    return 0;
}
