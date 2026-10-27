/* unpack.c — the decoder of packed streams, format 1 (#345, <unpack.h>,
 * Pack45.hpp): literals copied, fills and repeats done by DMA at 28-bit
 * addresses; read through a far pointer. In place, the literals are copied
 * downwards (the stream ahead of the writing): a forward DMA copy is right. */
#include <unpack.h>
#include <dma.h>

__fastcall__ unsigned long __unpack(unsigned long src) {
    const unsigned char __far *p = (const unsigned char __far *)src;
    unsigned long dst, start;
    unsigned char t;
    unsigned int n, o;
    if (p[0] != UNPACK_FORMAT) return 0;
    dst = p[1] | ((unsigned int)p[2] << 8) | ((unsigned long)p[3] << 16) | ((unsigned long)p[4] << 24);
    start = dst;
    p += 5;
    for (;;) {
        t = *p++;
        n = t & 0x3F;
        if (t < 0x40) {                 /* n + 1 bytes as they are */
            n += 1;
            dma_copy_far(dst, (unsigned long)p, n);
            p += n;
        } else if (t < 0x80) {          /* n + 3 bytes of one */
            n += 3;
            dma_fill_far(dst, n, *p++);
        } else {                        /* n + 3 bytes from o back */
            if (t < 0xC0) {
                o = *p++ + 1;
            } else {
                o = p[0] | ((unsigned int)p[1] << 8);
                p += 2;
                if (!n && !o) return dst - start;
            }
            n += 3;
            dma_copy_far(dst, dst - o, n);
        }
        dst += n;
    }
}
