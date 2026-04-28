/* stdint.h — Fixed-width integer types for cc45 / MEGA65 (C99)
 *
 * cc45 native types:
 *   char  = 8-bit unsigned
 *   int   = 16-bit (signed/unsigned)
 */

#pragma include_once

/* Exact-width types */
#define int8_t   signed char
#define uint8_t  unsigned char
#define int16_t  int
#define uint16_t unsigned int

/* Limits */
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255
#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535
