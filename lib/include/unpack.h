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
 * Format 1 (Pack45.hpp): the format byte, the destination (4 bytes), then
 * tokens: literals, fills (DMA fill) and repeats (DMA copy). unpack returns
 * the number of bytes it wrote, 0 for a stream of another format.
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
