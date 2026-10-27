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
 */
#ifndef _UNPACK_H
#define _UNPACK_H

#define UNPACK_FORMAT 1

/* (the ZP convention in either library: the overlay loader calls it from
   assembly). The library's name is __unpack, so a program's own function
   named unpack cannot take the overlay loader's call (#346) */
__fastcall__ unsigned long __unpack(unsigned long src);
#define unpack __unpack

#endif
