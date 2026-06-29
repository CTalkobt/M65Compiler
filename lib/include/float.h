#ifndef _FLOAT_H
#define _FLOAT_H

/*
 * float.h — Floating-point characteristics for cc45 (CBM 40-bit float)
 *
 * CBM 40-bit format: 1 byte exponent + 4 bytes mantissa (31 bits + implicit 1)
 * Exponent bias: 128 (excess-128)
 * No IEEE 754 — this is the Commodore BASIC float format.
 */

/* Radix of exponent representation */
#define FLT_RADIX       2

/* Number of digits in the significand (base FLT_RADIX) */
#define FLT_MANT_DIG    32

/* Number of decimal digits of precision */
#define FLT_DIG         9

/* Minimum negative exponent (base FLT_RADIX) */
#define FLT_MIN_EXP     (-127)

/* Maximum exponent (base FLT_RADIX) */
#define FLT_MAX_EXP     127

/* Minimum negative exponent (base 10) */
#define FLT_MIN_10_EXP  (-38)

/* Maximum exponent (base 10) */
#define FLT_MAX_10_EXP  38

/* Rounding mode: rounds to nearest */
#define FLT_ROUNDS      1

/* double = float on this target */
#define DBL_MANT_DIG    FLT_MANT_DIG
#define DBL_DIG         FLT_DIG
#define DBL_MIN_EXP     FLT_MIN_EXP
#define DBL_MAX_EXP     FLT_MAX_EXP
#define DBL_MIN_10_EXP  FLT_MIN_10_EXP
#define DBL_MAX_10_EXP  FLT_MAX_10_EXP

/* long double = float on this target */
#define LDBL_MANT_DIG   FLT_MANT_DIG
#define LDBL_DIG        FLT_DIG
#define LDBL_MIN_EXP    FLT_MIN_EXP
#define LDBL_MAX_EXP    FLT_MAX_EXP
#define LDBL_MIN_10_EXP FLT_MIN_10_EXP
#define LDBL_MAX_10_EXP FLT_MAX_10_EXP

/* Maximum representable finite float (exponent $FF, mantissa $FFFFFFFF) */
#define FLT_MAX     1.7014118e38

/* Minimum positive normalized float (exponent $01, mantissa $80000000) */
#define FLT_MIN     2.9387359e-39

/* Smallest x such that 1.0 + x != 1.0 (2^-31) */
#define FLT_EPSILON 4.6566129e-10

/* double = float on this target */
#define DBL_MAX     FLT_MAX
#define DBL_MIN     FLT_MIN
#define DBL_EPSILON FLT_EPSILON

/* long double = float on this target */
#define LDBL_MAX    FLT_MAX
#define LDBL_MIN    FLT_MIN
#define LDBL_EPSILON FLT_EPSILON

#endif /* _FLOAT_H */
