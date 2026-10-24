/* test_ca45_reloc_xemu.c — ca45 -c relocations, with test_ca45_reloc.s45:
 * a difference of two labels of one segment is a constant (#336), and a
 * "zp" segment label is a byte of the program's base page in every
 * base-page mode, its full address in an absolute one (#335)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] lda #<(blk_end-blk) / ldx #>(...): 3
 *   [1] lda blk+(blk_end-blk)-1: the block's last byte
 *   [2] .byte <(blk_end-blk), >(blk_end-blk): 3, 0
 *   [3] .word blk_end-blk: 3
 *   [4] .word blk+(blk_end-blk): blk_end's address
 *   [5] (zp),y and [zp],z through a zp segment pointer
 *   [6] inc zp, then (zp),y: the pointer moved on
 *   [7] zp,x: its high byte
 *   [8] ldz zp (absolute): the same byte as the base-page operand
 *   [9] lda #zp_end-ptr: 4
 *   [10] $AA sentinel
 */
#define R ((volatile unsigned char *)0xC0C0)

extern unsigned int blk_len(void);
extern unsigned char blk_last(void);
extern unsigned char tab[6];
extern unsigned char blk[], blk_end[];
extern unsigned char *zp_target;
extern unsigned char zp_out[6];
extern void zp_test(void);

_Alignas(16) static unsigned char buf[4] = {0x11, 0x22, 0x33, 0x44};

int main(void) {
    unsigned char i;
    for (i = 0; i < 11; i++) R[i] = 0;
    R[0] = blk_len() == 3;
    R[1] = blk_last() == 3;
    R[2] = tab[0] == 3 && tab[1] == 0;
    R[3] = (tab[2] | tab[3] << 8) == 3;
    R[4] = (unsigned int)(tab[4] | tab[5] << 8) == (unsigned int)blk_end;
    zp_target = buf;
    zp_test();
    R[5] = zp_out[0] == 0x11 && zp_out[1] == 0x22;
    R[6] = zp_out[2] == 0x33;
    R[7] = zp_out[3] == (unsigned char)((unsigned int)buf >> 8);
    R[8] = zp_out[4] == (unsigned char)((unsigned int)buf + 1);
    R[9] = zp_out[5] == 4;
    R[10] = 0xAA;
    return 0;
}
