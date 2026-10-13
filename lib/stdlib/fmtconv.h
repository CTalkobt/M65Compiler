/* fmtconv.h -- the printf formatter's digit hooks (library internal)
 *
 * __vformat fetches an integer argument and its sign itself and calls a
 * hook for the digits: __fmt_utoa (int and char arguments) or __fmt_ultoa
 * (long, long long). A hook writes the digits of v in base (2..16)
 * backwards, ending before end, each digits[remainder], and returns their
 * number (at least 1). __vformat has weak stubs that write none, and asks
 * __fmt_ultoa for an int argument when __fmt_utoa writes none, so linking
 * __fmt_ultoa alone covers both. stdlib_asm/fmt_utoa.s45 / fmt_ultoa.s45
 * define the real ones with a marker (__fmt_int_linked / __fmt_long_linked)
 * that cc45 references when a format string of the program needs them (the
 * long one for a format that is not a literal).
 */
#ifndef _FMTCONV_H
#define _FMTCONV_H
__fastcall__ int __fmt_utoa(unsigned int v, unsigned int base, char *end, char *digits);
__fastcall__ int __fmt_ultoa(unsigned long v, unsigned int base, char *end, char *digits);
#endif
