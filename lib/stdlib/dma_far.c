/* dma_far.c — DMA copy and fill at 28-bit addresses (dma.h: dma_copy_far,
 * dma_fill_far)
 *
 * An enhanced F018B DMA job list: options ($0B F018B format, $80/$81 the
 * source/destination megabyte, $00 end), then the 12-byte job whose bank
 * bytes hold address bits 16-19. Written to $D702/$D704/$D701 (list bank,
 * megabyte, high byte) and $D705, which starts an enhanced job. The CPU
 * waits for the DMA, so the copy is done when the call returns.
 *
 * Addresses are linear: $0000800 screen RAM, $FF80000 colour RAM,
 * $0040000 the fifth 64 KB bank of chip RAM, ...
 */
#include <dma.h>

#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char job[18] = {
    0x0B,          /* F018B job format */
    0x80, 0,       /* source address bits 20-27 */
    0x81, 0,       /* destination address bits 20-27 */
    0x00,          /* end of options */
    0,             /* command: 0 copy, 3 fill */
    0, 0,          /* length */
    0, 0, 0,       /* source address bits 0-15, bank (bits 16-19) */
    0, 0, 0,       /* destination address bits 0-15, bank */
    0,             /* sub-command */
    0, 0           /* modulo */
};

static void dma_far_run(unsigned char cmd, unsigned long dst, unsigned long src, unsigned int len) {
    if (len == 0) return;   /* a DMA length of 0 means 64 KB */
    job[2] = (unsigned char)(src >> 20);
    job[4] = (unsigned char)(dst >> 20);
    job[6] = cmd;
    job[7] = (unsigned char)len;
    job[8] = (unsigned char)(len >> 8);
    job[9] = (unsigned char)src;
    job[10] = (unsigned char)(src >> 8);
    job[11] = (unsigned char)(src >> 16) & 0x0F;
    job[12] = (unsigned char)dst;
    job[13] = (unsigned char)(dst >> 8);
    job[14] = (unsigned char)(dst >> 16) & 0x0F;
    REG(0xD702) = 0;                                   /* list in bank 0 */
    REG(0xD704) = 0;                                   /* megabyte 0 */
    REG(0xD701) = (unsigned int)job >> 8;
    REG(0xD705) = (unsigned int)job & 0xFF;            /* start (enhanced job) */
}

void __dma_copy_far(unsigned long dst, unsigned long src, unsigned int len) {
    dma_far_run(0x00, dst, src, len);
}

void __dma_fill_far(unsigned long dst, unsigned int len, unsigned char val) {
    dma_far_run(0x03, dst, val, len);   /* fill: the value is the source address low byte */
}
