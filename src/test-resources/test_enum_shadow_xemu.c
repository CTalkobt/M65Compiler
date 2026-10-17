/* test_enum_shadow_xemu.c — a declaration hides an enum constant (Issue #286)
 *
 * The parser put an enum constant's value in for its name everywhere, also
 * where a local or parameter of that name hid it.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a block's local; after the block the name is the constant again
 *   [1] a parameter
 *   [2] a for statement's variable (the constant again after the loop)
 *   [3] an enum defined in a block ends with it
 *   [4] the constant in the hiding variable's own declarator (int E[E])
 *   [5] a struct local and a function-pointer local
 *   [6] the constants unchanged at file scope
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

enum { E = 50, F = 60, G = 70, H = 9 };
struct S { unsigned char a, b; };

NOINLINE static unsigned char in_block(unsigned char c) {
    unsigned char s;
    { unsigned char E = c; s = E; }
    return s + E;                        /* c + 50 */
}

NOINLINE static unsigned char param(unsigned char F) { return F + 1; }

NOINLINE static unsigned char for_var(void) {
    unsigned char s = 0;
    for (unsigned char G = 0; G < 3; G++) s += G;
    return s + G;                        /* 3 + 70 */
}

NOINLINE static unsigned char block_enum(void) {
    unsigned char s;
    { enum { E = 5 }; s = E; }
    return s + E;                        /* 5 + 50 */
}

NOINLINE static unsigned char own_declarator(void) {
    unsigned char E[E / 10];
    return sizeof E;                     /* 5 */
}

static unsigned char two(void) { return 2; }

NOINLINE static unsigned char struct_local(unsigned char c) {
    unsigned char s;
    { struct S E; E.a = c; E.b = 1; s = E.a + E.b; }
    return s + E;                        /* c + 1 + 50 */
}

NOINLINE static unsigned char fnptr_local(void) {
    unsigned char (*H)(void) = two;
    return H();
}

int main(void) {
    OUT[0] = in_block(3) == 53;
    OUT[1] = param(7) == 8;
    OUT[2] = for_var() == 73;
    OUT[3] = block_enum() == 55;
    OUT[4] = own_declarator() == 5;
    OUT[5] = struct_local(3) == 54 && fnptr_local() == 2;
    OUT[6] = E == 50 && F == 60 && G == 70 && H == 9;
    OUT[7] = 0xAA;
    return 0;
}
