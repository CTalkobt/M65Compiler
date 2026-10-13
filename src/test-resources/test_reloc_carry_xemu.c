/* test_reloc_carry_xemu.c — high-byte relocations of "symbol + n" in a .o45
 * (linked with test_reloc_carry_asm_xemu.s45)
 *
 * #>sym+n is the high byte of sym+n; the linker can only carry into it
 * from the low byte if the object records that low byte (R_HIGH's extra
 * byte). With n = $1FF the high byte is wrong for almost any placement of
 * buf when the low byte is lost.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] lda #<buf+$1FF / ldx #>buf+$1FF
 *   [1] ldax #buf+$1FF
 *   [2] lda #<buf+2 / ldx #>buf+2
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int addr_bytes(void);
unsigned int addr_ldax(void);
unsigned int addr_small(void);
extern unsigned char buf[];

int main(void) {
    OUT[0] = addr_bytes() == (unsigned int)(buf + 0x1FF);
    OUT[1] = addr_ldax() == (unsigned int)(buf + 0x1FF);
    OUT[2] = addr_small() == (unsigned int)(buf + 2);
    OUT[3] = 0xAA;
    return 0;
}
