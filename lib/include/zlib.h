/* zlib.h — zlib's checksums for cc45 / MEGA65 (as cc65's <zlib.h>)
 *
 * crc32 (lib/stdlib_asm/crc32.s45): zlib's CRC-32. Start with 0 (or
 * crc32(0, NULL, 0)) and pass each piece of the data with the value so far:
 *   unsigned long c = crc32(0, NULL, 0);
 *   c = crc32(c, buf, len);
 * adler32 (lib/stdlib_asm/adler32.s45): zlib's Adler-32, used the same
 * way, starting from 1 (adler32(0, NULL, 0)).
 * cc65's inflatemem / uncompress are not here (pack45 and <unpack.h> are
 * the library's compression).
 */
#pragma once

#ifndef NULL
#define NULL 0
#endif

__fastcall__ unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned int len);
__fastcall__ unsigned long adler32(unsigned long adler, const unsigned char *buf, unsigned int len);
