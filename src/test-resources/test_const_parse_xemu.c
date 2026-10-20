/* test_const_parse_xemu.c — constant expressions the parser evaluates, through
 * ConstEval (Issue #93, stage 3)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] array sizes with casts: char a[(unsigned char)300] has 44 elements
 *       (it had 300); sizes with sizeof of a variable, ?: and shifts
 *   [1] enum values with casts ((unsigned char)300 is 44, (signed char)200
 *       is -56) and with sizeof of a variable
 *   [2] bit-field widths that are expressions (1 + 2, sizeof(char) * 4,
 *       (unsigned char)259): the fields hold that many bits
 *   [3] _Static_assert with sizeof of a variable (was "not a constant
 *       expression"), at file scope and in a function
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int x;
long lx;
char a1[(unsigned char)300];
char a2[sizeof lx > 2 ? sizeof x << 2 : 1];
char a3[(1 << 3) | sizeof x];
enum { E1 = (unsigned char)300, E2 = (signed char)200, E3 = sizeof lx * 3, E4 };
struct bits { unsigned w3 : 1 + 2; unsigned w4 : sizeof(char) * 4; unsigned w3b : (unsigned char)259; };
struct bits bf;
_Static_assert(sizeof x == 2, "int is 2 bytes");
_Static_assert(sizeof a1 == 44 && sizeof lx == 4, "sizes");

static unsigned char in_function(void) {
    char local[sizeof lx + 1];
    _Static_assert(sizeof local == 5, "local array");
    return (unsigned char)sizeof local;
}

int main(void) {
    OUT[0] = sizeof a1 == 44 && sizeof a2 == 8 && sizeof a3 == 10;
    OUT[1] = E1 == 44 && (signed int)E2 == -56 && E3 == 12 && E4 == 13;
    bf.w3 = 15;    /* keeps 3 bits: 7 */
    bf.w4 = 31;    /* 4 bits: 15 */
    bf.w3b = 255;  /* 3 bits: 7 */
    OUT[2] = bf.w3 == 7 && bf.w4 == 15 && bf.w3b == 7;
    OUT[3] = in_function() == 5;
    OUT[4] = 0xAA;
    return 0;
}
