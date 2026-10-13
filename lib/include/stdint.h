/* stdint.h — Fixed-width integer types for cc45 / MEGA65 (C99)
 *
 * Provides standard fixed-width integer types for portable code.
 * cc45 native types:
 *   char  = 8-bit
 *   int   = 16-bit (signed/unsigned)
 *   long  = 32-bit (signed/unsigned)
 */

#pragma once

/* ============================================================================
 * EXACT-WIDTH INTEGER TYPES
 * ============================================================================ */

typedef signed char         int8_t;
typedef unsigned char       uint8_t;

typedef signed int          int16_t;
typedef unsigned int        uint16_t;

typedef signed long         int32_t;
typedef unsigned long       uint32_t;

/* 64-bit types: the __int(64) wide integer (struct __int64 of <intwide.h>,
 * which cc45 includes in a unit that names a 64-bit type). Arithmetic is
 * through its operators; integers convert to it and it narrows to integers
 * (low bytes). uint64_t is the same struct.
 */
typedef struct __int64      int64_t;
typedef struct __int64      uint64_t;

/* ============================================================================
 * FASTEST TYPES (native word size)
 * ============================================================================ */

typedef signed char         int_fast8_t;
typedef unsigned char       uint_fast8_t;

typedef signed int          int_fast16_t;
typedef unsigned int        uint_fast16_t;

typedef signed long         int_fast32_t;
typedef unsigned long       uint_fast32_t;

typedef struct __int64      int_fast64_t;
typedef struct __int64      uint_fast64_t;

/* ============================================================================
 * LEAST-WIDTH TYPES (minimum size that fits)
 * ============================================================================ */

typedef signed char         int_least8_t;
typedef unsigned char       uint_least8_t;

typedef signed int          int_least16_t;
typedef unsigned int        uint_least16_t;

typedef signed long         int_least32_t;
typedef unsigned long       uint_least32_t;

typedef struct __int64      int_least64_t;
typedef struct __int64      uint_least64_t;

/* ============================================================================
 * POINTER AND MAXIMUM WIDTH TYPES
 * ============================================================================ */

typedef signed int          intptr_t;
typedef unsigned int        uintptr_t;

typedef signed long         intmax_t;
typedef unsigned long       uintmax_t;

/* ============================================================================
 * LIMITS OF FIXED-WIDTH INTEGER TYPES
 * ============================================================================ */

/* int8_t */
#define INT8_MIN            (-128)
#define INT8_MAX            127
#define UINT8_MAX           255

/* int16_t */
#define INT16_MIN           (-32768)
#define INT16_MAX           32767
#define UINT16_MAX          65535

/* int32_t */
#define INT32_MIN           (-2147483648L)
#define INT32_MAX           2147483647L
#define UINT32_MAX          4294967295UL

/* int64_t */
#define INT64_MIN           (-9223372036854775807LL - 1)
#define INT64_MAX           9223372036854775807LL
#define UINT64_MAX          18446744073709551615ULL

/* ============================================================================
 * LIMITS OF FASTEST FIXED-WIDTH TYPES
 * ============================================================================ */

#define INT_FAST8_MIN       INT8_MIN
#define INT_FAST8_MAX       INT8_MAX
#define UINT_FAST8_MAX      UINT8_MAX

#define INT_FAST16_MIN      INT16_MIN
#define INT_FAST16_MAX      INT16_MAX
#define UINT_FAST16_MAX     UINT16_MAX

#define INT_FAST32_MIN      INT32_MIN
#define INT_FAST32_MAX      INT32_MAX
#define UINT_FAST32_MAX     UINT32_MAX

#define INT_FAST64_MIN      INT64_MIN
#define INT_FAST64_MAX      INT64_MAX
#define UINT_FAST64_MAX     UINT64_MAX

/* ============================================================================
 * LIMITS OF LEAST FIXED-WIDTH TYPES
 * ============================================================================ */

#define INT_LEAST8_MIN      INT8_MIN
#define INT_LEAST8_MAX      INT8_MAX
#define UINT_LEAST8_MAX     UINT8_MAX

#define INT_LEAST16_MIN     INT16_MIN
#define INT_LEAST16_MAX     INT16_MAX
#define UINT_LEAST16_MAX    UINT16_MAX

#define INT_LEAST32_MIN     INT32_MIN
#define INT_LEAST32_MAX     INT32_MAX
#define UINT_LEAST32_MAX    UINT32_MAX

#define INT_LEAST64_MIN     INT64_MIN
#define INT_LEAST64_MAX     INT64_MAX
#define UINT_LEAST64_MAX    UINT64_MAX

/* ============================================================================
 * LIMITS OF POINTER AND MAXIMUM WIDTH TYPES
 * ============================================================================ */

#define INTPTR_MIN          INT16_MIN
#define INTPTR_MAX          INT16_MAX
#define UINTPTR_MAX         UINT16_MAX

#define INTMAX_MIN          INT32_MIN
#define INTMAX_MAX          INT32_MAX
#define UINTMAX_MAX         UINT32_MAX

/* ============================================================================
 * MACROS FOR FIXED-WIDTH INTEGER CONSTANTS
 * ============================================================================ */

#define INT8_C(c)           (c)
#define UINT8_C(c)          (c##U)

#define INT16_C(c)          (c)
#define UINT16_C(c)         (c##U)

#define INT32_C(c)          (c##L)
#define UINT32_C(c)         (c##UL)

#define INT64_C(c)          (c##LL)
#define UINT64_C(c)         (c##ULL)

#define INTMAX_C(c)         (c##L)
#define UINTMAX_C(c)        (c##UL)
