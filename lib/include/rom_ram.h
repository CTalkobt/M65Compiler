/* rom_ram.h — unused parts of the ROM area ($20000-$3FFFF) as RAM (#339)
 *
 * In C65 mode some of the ROM image in chip RAM is never used: the C64
 * BASIC and KERNAL. The hypervisor write-protects the whole area
 * ($D641 trap); romram_enable lifts the protection and checks that a
 * region now takes writes, romram_protect puts it back (so does the
 * program's exit, crt0's __exit).
 *
 *   if (romram_enable(ROMRAM_C64)) {
 *       const unsigned long *r = romram_ranges(ROMRAM_C64);   // 2, then
 *       ...                                                     // first/last pairs
 *       dma_copy_far(r[1], buffer, 8192);                       // 28-bit addresses
 *   }
 *
 * The C64 region is two 8 KB pieces: $2A000-$2BFFF (C64 BASIC) and
 * $2E000-$2FFFF (the end of C64 BASIC and the C64 KERNAL); $2C000-$2DFFF
 * between them holds parts of the C65 DOS and KERNAL and a character set
 * C65 mode uses. Overwriting the region breaks only GO64 (until the ROM is
 * loaded again: a reset). The ranges are 28-bit addresses: reach them with
 * DMA (<dma.h> dma_copy_far / dma_fill_far) or far pointers.
 */
#ifndef ROM_RAM_H
#define ROM_RAM_H

#define ROMRAM_C64  1   /* C64 BASIC $2A000-$2BFFF, C64 BASIC end + KERNAL $2E000-$2FFFF (16 KB) */

/* Writes to the ROM area on; 1 when each of the regions (ROMRAM_... bits)
 * takes a write (a byte of each changed and read back, then restored), 0
 * otherwise (an unknown region bit, or the protection stayed) */
__fastcall__ unsigned char romram_enable(unsigned char regions);

/* Writes to the ROM area off again (the contents stay as they are) */
__fastcall__ void romram_protect(void);

/* A region's ranges: their count, then first / last address pairs (28-bit);
 * NULL for an unknown region */
__fastcall__ const unsigned long *romram_ranges(unsigned char region);

#endif
