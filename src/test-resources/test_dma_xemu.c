/* test_dma_xemu.c — dma_fill / dma_copy (F018B list built on the stack)
 *
 * Results at $C0C0: [0] fill value and length, [1] copy, [2] $AA
 */
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char buf[20];
static unsigned char dst[20];

int main(void) {
    unsigned char i;
    for (i = 0; i < 20; i++) buf[i] = i + 1;
    dma_fill(buf, 10, 0x5A);
    dma_copy(dst, buf, 20);
    OUT[0] = buf[0] == 0x5A && buf[9] == 0x5A && buf[10] == 11;
    OUT[1] = dst[0] == 0x5A && dst[19] == 20;
    OUT[2] = 0xAA;
    return 0;
}
