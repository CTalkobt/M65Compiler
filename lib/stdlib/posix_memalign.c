/* posix_memalign.c — malloc'd memory on an alignment (<stdlib.h>)
 *
 * malloc.s45's blocks start at even addresses with a 2-byte header (the
 * block's size, header included, bit 0 set while allocated). For a larger
 * alignment a block with room to spare is split: its first part, at least
 * 4 bytes up to the aligned address, is freed again.
 */

#include <stdlib.h>
#include <errno.h>

#define HDR(p) (*(unsigned int *)((unsigned char *)(p) - 2))

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    unsigned char *raw;
    unsigned int a, gap, blk;
    if (alignment < 2 || (alignment & (alignment - 1))) return EINVAL;
    if (size > 0xFFF0u - alignment) return ENOMEM;
    if (alignment == 2) {
        raw = (unsigned char *)malloc(size ? size : 1);
        if (!raw) return ENOMEM;
        *memptr = raw;
        return 0;
    }
    raw = (unsigned char *)malloc((size ? size : 1) + alignment + 4);
    if (!raw) return ENOMEM;
    if (!((unsigned int)raw & (alignment - 1))) {
        *memptr = raw;
        return 0;
    }
    a = ((unsigned int)raw + 4 + alignment - 1) & ~(alignment - 1);
    gap = a - (unsigned int)raw;
    blk = HDR(raw) & 0xFFFEu;
    HDR(a) = (blk - gap) | 1;
    HDR(raw) = gap | 1;
    free(raw);
    *memptr = (void *)a;
    return 0;
}
