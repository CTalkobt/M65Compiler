/* dma.h — MEGA65 DMA operations for cc45
 *
 * Provides DMA copy and fill operations using the MEGA65's F018B DMA
 * controller (~40MB/s vs ~1MB/s for CPU loops).
 *
 * Usage:
 *   #include <dma.h>
 *   dma_copy(dst, src, len);   // DMA memory copy
 *   dma_fill(dst, len, val);   // DMA memory fill
 *
 * These are compiler builtins — no library linkage needed.
 * The DMA job is built on the hardware stack and triggered inline.
 * All registers are preserved across the call.
 *
 * Limitations:
 *   - src/dst must be 16-bit addresses (bank 0 only)
 *   - len is 16-bit (max 65535 bytes per call)
 *   - val is 8-bit fill value
 *   - Not safe to use inside interrupts (uses hardware stack for job)
 */

#ifndef _DMA_H
#define _DMA_H

/* Public API */
#define dma_copy(dst, src, len)  __dma_copy((dst), (src), (len))
#define dma_fill(dst, len, val)  __dma_fill((dst), (len), (val))

/* Internal compiler builtins — prefer the macros above */
void __dma_copy(void *dst, const void *src, unsigned int len);
void __dma_fill(void *dst, unsigned int len, unsigned char val);

#endif /* _DMA_H */
