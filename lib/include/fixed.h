/* fixed.h — fixed-point types (cc45, #72)
 *
 * __fixed(I, F)  a signed number of I + F bits (16 or 32), F of them the
 *                fraction: __fixed(4, 28) holds -8 to 7.99999999 in steps
 *                of 2^-28, __fixed(1, 15) -1 to 0.99997 (audio)
 * __fixed8_8     __fixed(8, 8): -128 to 127.996, steps of 1/256
 * __fixed16_16   __fixed(16, 16): -32768 to 32767.99998, steps of 1/65536
 *
 * cc45 knows the types (no include needed; this header comes with them):
 * constants are scaled when compiled (__fixed8_8 x = 1.5 is $0180), + - and
 * comparisons are integer code, * and / scale (16-bit ones inline, 32-bit
 * ones by __fx32_mul / __fx32_div on the hardware multiplier and divider),
 * and int, float and other fixed types convert to and from them.
 *
 * printf has no conversion of its own for them: print a float,
 *   printf("%f", toFloat8_8(x));       printf("%f", fixedToFloat(z));
 */
#pragma once

/* the integers of two fixed values of F fraction bits: (a * b) >> f and
 * (a << f) / b (cc45 calls them for * and / of 32-bit fixed types) */
__fastcall__ signed long __fx32_mul(signed long a, signed long b, unsigned char f);
__fastcall__ signed long __fx32_div(signed long a, signed long b, unsigned char f);

/* Any fixed value as a float, or as an integer (the fraction dropped, toward 0) */
#define fixedToFloat(v) ((float)(v))
#define fixedToInt(v)   ((signed long)(v))
/* the same for the two named types (v converted to the type first) */
#define toFloat8_8(v)   ((float)(__fixed8_8)(v))
#define toFloat16_16(v) ((float)(__fixed16_16)(v))
#define toInt8_8(v)     ((signed int)(__fixed8_8)(v))
#define toInt16_16(v)   ((signed long)(__fixed16_16)(v))
