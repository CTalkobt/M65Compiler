/* memcpy_fast.c — Optimized memory copy with loop unrolling
 *
 * Fast-path memcpy for common cases (aligned blocks, power-of-2 sizes).
 * Falls back to standard memcpy for unusual cases.
 *
 * Performance optimizations:
 * - Loop unrolling (4-byte copies)
 * - Alignment detection and fast path
 * - Register-resident loop counters
 */

#include <string.h>

/* Standard memcpy from asm for fallback and small blocks */
extern void *memcpy(void *dest, const void *src, int n);

/* Fast unrolled copy for 4-byte aligned blocks (common case) */
static inline void *memcpy_aligned_4(char *dest, const char *src, signed int n)
    __attribute__((always_inline)) {
    #pragma cc45 optimize(loop-unrolling, strength-reduction)

    /* Unroll by 4: copy 4 bytes at a time */
    signed int count = n / 4;
    signed int remainder = n % 4;

    /* Fast path: 4-byte copies */
    for (int i = 0; i < count; i++) {
        dest[i * 4 + 0] = src[i * 4 + 0];
        dest[i * 4 + 1] = src[i * 4 + 1];
        dest[i * 4 + 2] = src[i * 4 + 2];
        dest[i * 4 + 3] = src[i * 4 + 3];
    }

    /* Handle remainder */
    char *d = dest + count * 4;
    const char *s = src + count * 4;
    for (int i = 0; i < remainder; i++) {
        d[i] = s[i];
    }

    return dest;
}

/* Fast path selector — helps compiler branch predict common case */
void *memcpy_optimized(void *dest, const void *src, int n) {
    #pragma cc45 optimize(branch-folding, constant-folding)

    /* Handle zero-length and tiny copies with standard path */
    if (n <= 4) {
        return memcpy(dest, src, n);
    }

    /* Detect alignment: if both pointers aligned to 4 bytes, use fast path */
    unsigned long d_addr = (unsigned long)dest;
    unsigned long s_addr = (unsigned long)src;

    if ((d_addr & 3) == 0 && (s_addr & 3) == 0) {
        return memcpy_aligned_4((char *)dest, (const char *)src, n);
    }

    /* Fall back to standard memcpy for unaligned cases */
    return memcpy(dest, src, n);
}
