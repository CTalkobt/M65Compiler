/* dma.h — MEGA65 DMA operations for cc45
 *
 * Provides DMA copy and fill operations using the MEGA65's F018B DMA
 * controller (~40MB/s vs ~1MB/s for CPU loops).
 *
 * Usage:
 *   #include <dma.h>
 *   dma_copy(dst, src, len);   // DMA memory copy
 *   dma_fill(dst, len, val);   // DMA memory fill
 *   dma_copy_far(dst, src, len);   // at 28-bit linear addresses (library)
 *   dma_fill_far(dst, len, val);
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
 *
 * dma_copy_far / dma_fill_far (the library's lib/stdlib_asm/dma_far.s45) take
 * 28-bit linear addresses as unsigned long: any chip RAM ($0000000-$005FFFF;
 * $0020000-$003FFFF holds the ROM), colour RAM at $FF80000 (all of it,
 * not only the $D800 window), ... An enhanced F018B job (megabyte options)
 * runs while the CPU waits. len is 16-bit; 0 does nothing; the copy runs
 * forwards (overlapping ranges: copy downwards only).
 */

#ifndef _DMA_H
#define _DMA_H

/* Public API */
#define dma_copy(dst, src, len)  __dma_copy((dst), (src), (len))
#define dma_fill(dst, len, val)  __dma_fill((dst), (len), (val))
#define dma_copy_far(dst, src, len)  __dma_copy_far((dst), (src), (len))
#define dma_fill_far(dst, len, val)  __dma_fill_far((dst), (len), (val))

/* Internal compiler builtins — prefer the macros above */
void __dma_copy(void *dst, const void *src, unsigned int len);
void __dma_fill(void *dst, unsigned int len, unsigned char val);

/* Library functions: 28-bit linear addresses (lib/stdlib_asm/dma_far.s45:
   the ZP convention in either) */
__fastcall__ void __dma_copy_far(unsigned long dst, unsigned long src, unsigned int len);
__fastcall__ void __dma_fill_far(unsigned long dst, unsigned int len, unsigned char val);

#endif /* _DMA_H */
