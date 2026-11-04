/* bitset.h — arrays of bits indexed at run time (#408)
 *
 *   BITSET(free_slots, 200);           unsigned char free_slots[25]
 *   bs_clear_all(free_slots, 200);
 *   bs_set(free_slots, i);
 *   if (bs_test(free_slots, j)) ...
 *   n = bs_find_first_clear(free_slots, 200, 0);    the index, or -1
 *
 * Bit i is in byte i >> 3, bit i & 7. A constant index makes the bit
 * operations an expression on that byte (one ora / and / eor); another index
 * calls the library (assembly, stdlib_asm/bitset.s45). The functions over
 * all of it take the number of bits and keep the bits past it in the last
 * byte 0 (start with bs_clear_all or a zeroed array). Up to 32767 bits.
 * (cc45's __bit / __bits are for named flags; this is for arrays.)
 */
#ifndef _BITSET_H
#define _BITSET_H

#define BITSET(name, nbits) unsigned char name[((nbits) + 7) >> 3]
#define BS_BYTES(nbits)     (((nbits) + 7) >> 3)

__fastcall__ void __bs_set(unsigned char *b, unsigned int i);
__fastcall__ void __bs_clear(unsigned char *b, unsigned int i);
__fastcall__ void __bs_toggle(unsigned char *b, unsigned int i);
__fastcall__ int __bs_test(const unsigned char *b, unsigned int i);

#define __BS_MASK(i) ((unsigned char)(1 << ((i) & 7)))
#define bs_set(b, i)    (__builtin_constant_p(i) ? (void)((b)[(i) >> 3] |= __BS_MASK(i)) : __bs_set((b), (i)))
#define bs_clear(b, i)  (__builtin_constant_p(i) ? (void)((b)[(i) >> 3] &= (unsigned char)~__BS_MASK(i)) : __bs_clear((b), (i)))
#define bs_toggle(b, i) (__builtin_constant_p(i) ? (void)((b)[(i) >> 3] ^= __BS_MASK(i)) : __bs_toggle((b), (i)))
#define bs_test(b, i)   (__builtin_constant_p(i) ? (((b)[(i) >> 3] & __BS_MASK(i)) != 0) : __bs_test((b), (i)))
#define bs_assign(b, i, v) ((v) ? bs_set((b), (i)) : bs_clear((b), (i)))

/* all nbits set / cleared / inverted */
__fastcall__ void bs_set_all(unsigned char *b, unsigned int nbits);
__fastcall__ void bs_clear_all(unsigned char *b, unsigned int nbits);
__fastcall__ void bs_not(unsigned char *b, unsigned int nbits);
/* bits from .. from + n - 1 */
__fastcall__ void bs_set_range(unsigned char *b, unsigned int from, unsigned int n);
__fastcall__ void bs_clear_range(unsigned char *b, unsigned int from, unsigned int n);
/* how many are set; whether any is */
__fastcall__ unsigned int bs_count(const unsigned char *b, unsigned int nbits);
__fastcall__ int bs_any(const unsigned char *b, unsigned int nbits);
#define bs_none(b, nbits) (!bs_any((b), (nbits)))
/* the first set / clear bit at from or after, below nbits: its index, or -1 */
__fastcall__ signed int bs_find_first_set(const unsigned char *b, unsigned int nbits, unsigned int from);
__fastcall__ signed int bs_find_first_clear(const unsigned char *b, unsigned int nbits, unsigned int from);
/* dst = dst op src, over nbits */
__fastcall__ void bs_and(unsigned char *dst, const unsigned char *src, unsigned int nbits);
__fastcall__ void bs_or(unsigned char *dst, const unsigned char *src, unsigned int nbits);
__fastcall__ void bs_xor(unsigned char *dst, const unsigned char *src, unsigned int nbits);

#endif
