/* stdint.h — Fixed-width integer types for cc45 / MEGA65 (C99)
 *
 * cc45 native types:
 *   char  = 8-bit unsigned
 *   int   = 16-bit (signed/unsigned)
 *   long  = 32-bit (signed/unsigned)
 */

#pragma once

/* Exact-width types */
#define int8_t   signed char
#define uint8_t  unsigned char
#define int16_t  int
#define uint16_t unsigned int
#define int32_t  long
#define uint32_t unsigned long

/* 64-bit types (mapped to long until full I64 codegen is available) */
#define int64_t  long
#define uint64_t unsigned long

/* Pointer-width types */
#define intptr_t  int
#define uintptr_t unsigned int

/* Limits */
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535
#define INT32_MIN   (-2147483648L)
#define INT32_MAX   2147483647L
#define UINT32_MAX  4294967295UL
