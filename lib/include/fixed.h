/* fixed.h — fixed-point types (cc45, #72)
 *
 * __fixed8_8   a signed number in 16 bits, 8 of them the fraction
 *              (-128 to 127.996, steps of 1/256)
 * __fixed16_16 a signed number in 32 bits, 16 of them the fraction
 *              (-32768 to 32767.99998, steps of 1/65536)
 *
 * cc45 knows the types (no include needed; this header comes with them):
 * constants are scaled when compiled (__fixed8_8 x = 1.5 is $0180), + - and
 * comparisons are integer code, * and / scale (8.8 inline, 16.16 by
 * __fx16_mul / __fx16_div on the hardware multiplier and divider), and
 * int, float and the other fixed type convert to and from them.
 *
 * printf has no conversion of its own for them: print a float,
 *   printf("%f", toFloat8_8(x));
 */
#pragma once

__fastcall__ __fixed16_16 __fx16_mul(__fixed16_16 a, __fixed16_16 b);
__fastcall__ __fixed16_16 __fx16_div(__fixed16_16 a, __fixed16_16 b);

/* The value as a float, or as an integer (the fraction dropped, toward 0) */
#define toFloat8_8(v)   ((float)(__fixed8_8)(v))
#define toFloat16_16(v) ((float)(__fixed16_16)(v))
#define toInt8_8(v)     ((signed int)(__fixed8_8)(v))
#define toInt16_16(v)   ((signed long)(__fixed16_16)(v))
