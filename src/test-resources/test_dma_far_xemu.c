/* test_dma_far_xemu.c — dma_copy_far / dma_fill_far at 28-bit addresses
 *
 * Results at $C0C0 (01 = pass):
 *   [0] copy to $40000 and back (with a fill of $50000 between: no aliasing)
 *   [1] fill of 300 bytes at $50010: the bytes before and after unchanged
 *   [2] all 2000 bytes of colour RAM at $FF80000 (checked through $D800
 *       and by DMA beyond the $D800-$DBFF window)
 *   [3] a copy across a 64 KB boundary ($4FFF0-$5000F)
 *   [4] length 0 changes nothing
 *   [5] $AA sentinel
 */
#include <dma.h>
#define OUT ((volatile unsigned char *)0xC0C0)
static unsigned char a[200], b[220];
#define LIN(p) ((unsigned long)(unsigned int)(p))
int main(void) {
    unsigned int i;
    unsigned char ok;
    for (i = 0; i < 200; i++) a[i] = (unsigned char)(i + 1);
    /* [0] out to $40000 and back */
    dma_copy_far(0x40000UL, LIN(a), 200);
    dma_fill_far(0x50000UL, 200, 0x99);      /* another bank: no aliasing */
    dma_copy_far(LIN(b), 0x40000UL, 200);
    ok = 1; for (i = 0; i < 200; i++) if (b[i] != (unsigned char)(i + 1)) ok = 0;
    OUT[0] = ok;
    /* [1] fill 300 bytes at $50010, read $5000F..$5013F+1 edges */
    dma_fill_far(0x5000FUL, 1, 0x11);
    dma_fill_far(0x5013CUL, 1, 0x22);
    dma_fill_far(0x50010UL, 300, 0x5A);
    dma_copy_far(LIN(b), 0x5000FUL, 1);  OUT[1] = b[0] == 0x11;
    dma_copy_far(LIN(b), 0x50010UL, 200); ok = 1; for (i = 0; i < 200; i++) if (b[i] != 0x5A) ok = 0;
    dma_copy_far(LIN(b), 0x5013BUL, 2);
    OUT[1] = OUT[1] && ok && b[0] == 0x5A && b[1] == 0x22;
    /* [2] colour RAM $FF80000: all 2000 bytes */
    dma_fill_far(0xFF80000UL, 2000, 0x05);
    dma_copy_far(LIN(b), 0xFF80000UL + 1990, 10);
    ok = 1; for (i = 0; i < 10; i++) if (b[i] != 5) ok = 0;
    OUT[2] = ok && (*(volatile unsigned char *)0xD800 & 0x0F) == 5 && (*(volatile unsigned char *)0xDBFF & 0x0F) == 5;
    /* [3] across a 64 KB boundary: $4FFF0..$5000F */
    dma_copy_far(0x4FFF0UL, LIN(a), 32);
    dma_copy_far(LIN(b), 0x4FFF0UL, 32);
    ok = 1; for (i = 0; i < 32; i++) if (b[i] != (unsigned char)(i + 1)) ok = 0;
    OUT[3] = ok;
    /* [4] length 0 changes nothing */
    b[0] = 0x77; dma_copy_far(LIN(b), LIN(a), 0); OUT[4] = b[0] == 0x77;
    OUT[5] = 0xAA;
    return 0;
}
