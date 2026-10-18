/* test_byte_imm_xemu.c — constants of byte operations as bytes (Issue #288)
 *
 * p & ~0x80 on an unsigned char was narrowed to an 8-bit AND with the
 * 16-bit constant -129 (and #-129: ca45 warned "8-bit operand overflow").
 * The constants are now the bytes; the results are unchanged.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] p & ~0x80, p & ~0x01
 *   [1] p | ~0x7F, p ^ ~0
 *   [2] p + -3, p - 300
 *   [3] p * -3
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))
#define FLAG 0x80

unsigned char r1, r2;

NOINLINE static void masks(unsigned char p) { r1 = p & ~FLAG; r2 = p & ~0x01; }
NOINLINE static void bits(unsigned char p) { r1 = p | ~0x7F; r2 = p ^ ~0; }
NOINLINE static void sums(unsigned char p) { r1 = p + -3; r2 = p - 300; }
NOINLINE static unsigned char prod(unsigned char p) { return p * -3; }

int main(void) {
    masks(0xC1);
    OUT[0] = r1 == 0x41 && r2 == 0xC0;
    bits(0x15);
    OUT[1] = r1 == 0x95 && r2 == 0xEA;
    sums(0x40);
    OUT[2] = r1 == 0x3D && r2 == (unsigned char)(0x40 - 300);
    OUT[3] = prod(5) == (unsigned char)(5 * 253);
    OUT[4] = 0xAA;
    return 0;
}
