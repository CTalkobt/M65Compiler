/* limits.h -- Implementation-defined limits for cc45 / MEGA65 (C99)
 *
 * cc45 type model:
 *   char           = 8-bit unsigned (default)
 *   signed char    = 8-bit signed
 *   int / short    = 16-bit unsigned (default)
 *   signed int     = 16-bit signed
 *   long           = 32-bit unsigned (default)
 *   signed long    = 32-bit signed
 */

#pragma once

/* Number of bits in a char */
#define CHAR_BIT    8

/* signed char */
#define SCHAR_MIN   (-128)
#define SCHAR_MAX   127

/* unsigned char */
#define UCHAR_MAX   255

/* char (unsigned on cc45) */
#define CHAR_MIN    0
#define CHAR_MAX    255

/* short (== int on cc45, 16-bit) */
#define SHRT_MIN    (-32768)
#define SHRT_MAX    32767
#define USHRT_MAX   65535

/* int (16-bit) */
#define INT_MIN     (-32768)
#define INT_MAX     32767
#define UINT_MAX    65535

/* long (32-bit) */
#define LONG_MIN    (-2147483648L)
#define LONG_MAX    2147483647L
#define ULONG_MAX   4294967295UL

/* Minimum-width limits (same as exact-width on this target) */
#define MB_LEN_MAX  1
