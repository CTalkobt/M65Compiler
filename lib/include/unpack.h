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
 * tokens: literals, fills (DMA fill) and repeats (DMA copy). unpack returns
 * the number of bytes it wrote, 0 for a stream of another format.
 *
 * Format 2 (#666; pack45 --image): an image in up to 16 colours, w (even, up
 * to 256) x h pixels, each coded against the pixel above or to its left; it
 * unpacks to h rows of w / 2 bytes, 4 bits a pixel (the left one the low
 * nibble), each the pixel's palette entry. Its decoder is a library member
 * of its own (0.9 KB of BSS): a program that unpacks format 2 streams says
 * so once, at file scope in any one of its files,
 *
 *   UNPACK_IMAGES;
 *
 * and unpack / unpack_to (and load_unpack) then take both formats; without
 * it they return 0 for a format 2 stream. Format 2 unpacks in place too (a
 * row is written after its codes are read): pack45 prints how far in.
 *
 * Assembly (stdlib_asm/unpack.s45), from a program's own .s45 too: the
 * stream's address in $10-$13 (unpack_to: the destination in $14-$17), then
 *   jsr ___unpack          (jsr ___unpack_to)
 * the count comes back in Q (A, X, Y, Z: bits 0-7 ... 24-31); $08-$19 and
 * the registers are changed.
 */
#ifndef _UNPACK_H
#define _UNPACK_H

#define UNPACK_FORMAT 1
#define UNPACK_FORMAT_IMAGE 2

/* links format 2's decoder (stdlib_asm/unpack2.s45): at file scope, once */
#define UNPACK_IMAGES __asm__(".extern ___unpack_images")

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
