/* test_reloc_simops_xemu.c — ca45 simulated instructions and Q
 * instructions on addresses in a relocatable segment
 * (linked with test_reloc_simops_asm_xemu.s45)
 *
 * The data sits at offsets below $100 of its segment: ca45 used to assemble
 * cmp.16/add.16/mul.16/... operands as that offset without a relocation (and
 * in base-page mode), and put the relocation of ldq/stq/adcq abs on the $42
 * prefix instead of the operand.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] cmp.16 .AX, mem      [1] add.16 .AX, mem     [2] mul.16 .AX, mem
 *   [3] mul.s16 .AX, mem     [4] and.16 .AX, mem     [5] clc/ldq/adcq/stq abs
 *   [6] cmp.32 .AXYZ, mem    [7] bfins mem           [8] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
unsigned int t_cmp16(void);
unsigned int t_add16(void);
unsigned int t_mul16(void);
unsigned int t_muls16(void);
unsigned int t_and16(void);
unsigned int t_cmp32(void);
void t_q(void);
void t_bf(void);
extern unsigned long res32;
extern unsigned char bits;

int main(void) {
    OUT[0] = t_cmp16() == 1;
    OUT[1] = t_add16() == 1007;
    OUT[2] = t_mul16() == 7000;
    OUT[3] = t_muls16() == (unsigned int)-7000;
    OUT[4] = t_and16() == 1000;
    t_q();
    OUT[5] = res32 == 100005UL;
    OUT[6] = t_cmp32() == 1;
    t_bf();
    OUT[7] = bits == (0x81 | (5 << 2));
    OUT[8] = 0xAA;
    return 0;
}
