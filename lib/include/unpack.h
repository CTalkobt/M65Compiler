/* unpack.h — packed data (#345): streams pack45 (or ln45 --pack-overlays)
 * makes, unpacked by DMA to any 28-bit address (chip RAM, attic RAM).
 *
 *   unsigned long n = unpack(at);   at: where the stream is (28 bits);
 *                                   it unpacks to the address in its header
 *
 * A stream can be loaded inside the area it unpacks into, as far in as
 * pack45 reports ("in place"), and unpacked there: the writing never
 * overtakes the reading. pack45 writes that load address in front of it, so
 *
 *   cbm_load_far("TILES", 5, 8, 0x41234UL);    the in-place address pack45 printed
 *   unpack(0x41234UL);                         unpacks to its destination ($40000)
 *
 * <load.h>'s load_unpack does both in one call, also for a stream loaded to
 * attic RAM: load_unpack("tiles", 5, 0, 0x41234UL).
 *
 * Format 1 (Pack45.hpp): the format byte, the destination (4 bytes), then
 * tokens: literals, fills (DMA fill) and repeats (DMA copy). Format 2 (#666;
 * pack45 --image): an image in up to 16 colours, w (even, up to 256) x h
 * pixels, each coded against the pixel above or to its left; it unpacks to
 * h rows of w / 2 bytes, 4 bits a pixel (the left one the low nibble), each
 * the pixel's palette entry. Both unpack in place. unpack returns the number
 * of bytes it wrote, 0 for a stream of another format.
 *
 * Each format's decoder is a library member of its own, linked only when the
 * program asks for it (#666): at file scope, in any one of its files,
 *
 *   UNPACK_FORMAT1;         format 1 (stdlib_asm/unpack1.s45, 315 bytes)
 *   UNPACK_FORMAT2;         format 2 (stdlib_asm/unpack2.s45, 581 bytes and
 *                           0.9 KB of BSS); UNPACK_IMAGES is the same
 *
 * unpack / unpack_to (and load_unpack) return 0 for a stream whose decoder is
 * not linked. A program that names none gets format 1: this header names it
 * itself, unless the program defines UNPACK_EXPLICIT_FORMATS before including
 * it (or <load.h>) in each of its files (or with cc45 -DUNPACK_EXPLICIT_FORMATS)
 * and names its formats: a program of format 2 streams alone then links no
 * format 1 decoder. ln45 --pack-overlays' files are format 1: their loader
 * links its decoder itself; so does font_load.
 *
 * Assembly (stdlib_asm/unpack.s45), from a program's own .s45 too (which
 * names the decoders it needs: .extern ___unpack_format1): the
 * stream's address in $10-$13 (unpack_to: the destination in $14-$17), then
 *   jsr ___unpack          (jsr ___unpack_to)
 * the count comes back in Q (A, X, Y, Z: bits 0-7 ... 24-31); $08-$19 and
 * the registers are changed.
 */
#ifndef _UNPACK_H
#define _UNPACK_H

#define UNPACK_FORMAT 1
#define UNPACK_FORMAT_IMAGE 2

/* link a format's decoder: at file scope, once in the program */
#define UNPACK_FORMAT1 __asm__(".extern ___unpack_format1")
#define UNPACK_FORMAT2 __asm__(".extern ___unpack_format2")
#define UNPACK_IMAGES UNPACK_FORMAT2

/* by default format 1, as before formats were chosen */
#ifndef UNPACK_EXPLICIT_FORMATS
UNPACK_FORMAT1;
#endif

/* (the ZP convention in either library: the overlay loader calls it from
   assembly). The library's name is __unpack, so a program's own function
   named unpack cannot take the overlay loader's call (#346) */
__fastcall__ unsigned long __unpack(unsigned long src);
#define unpack __unpack

/* The same to dst, the stream's own destination ignored: a stream unpacked
   to more than one place, or made with any -d */
__fastcall__ unsigned long __unpack_to(unsigned long src, unsigned long dst);
#define unpack_to __unpack_to

#endif
